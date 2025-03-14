/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScraperUrl.h"

#include "CharsetConverter.h"
#include "ServiceBroker.h"
#include "URIUtils.h"
#include "URL.h"
#include "XMLUtils.h"
#include "filesystem/CurlFile.h"
#include "filesystem/ZipFile.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/CharsetDetection.h"
#include "utils/Mime.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <algorithm>
#include <cstring>
#include <sstream>

CScraperUrl::CScraperUrl() : m_relevance(0.0), m_parsed(false)
{
}

CScraperUrl::CScraperUrl(const std::string& strUrl) : CScraperUrl()
{
  ParseFromData(strUrl);
}

CScraperUrl::CScraperUrl(const TiXmlElement* element) : CScraperUrl()
{
  ParseAndAppendUrl(element);
}

CScraperUrl::~CScraperUrl() = default;

void CScraperUrl::Clear()
{
  m_urls.clear();
  m_data.clear();
  m_relevance = 0.0;
  m_parsed = false;
}

void CScraperUrl::SetData(std::string data)
{
  m_data = std::move(data);
  m_parsed = false;
}

const CScraperUrl::SUrlEntry CScraperUrl::GetFirstUrlByType(const std::string& type) const
{
  const auto url = std::find_if(m_urls.begin(), m_urls.end(), [type](const SUrlEntry& url) {
    return url.m_type == UrlType::General && (type.empty() || url.m_aspect == type);
  });
  if (url != m_urls.end())
    return *url;

  return SUrlEntry();
}

const CScraperUrl::SUrlEntry CScraperUrl::GetSeasonUrl(int season, const std::string& type) const
{
  const auto url = std::find_if(m_urls.begin(), m_urls.end(), [season, type](const SUrlEntry& url) {
    return url.m_type == UrlType::Season && url.m_season == season &&
           (type.empty() || type == "thumb" || url.m_aspect == type);
  });
  if (url != m_urls.end())
    return *url;

  return SUrlEntry();
}

unsigned int CScraperUrl::GetMaxSeasonUrl() const
{
  unsigned int maxSeason = 0;
  for (const auto& url : m_urls)
  {
    if (url.m_type == UrlType::Season && url.m_season > 0 &&
        static_cast<unsigned int>(url.m_season) > maxSeason)
      maxSeason = url.m_season;
  }
  return maxSeason;
}

std::string CScraperUrl::GetFirstThumbUrl() const
{
  if (m_urls.empty())
    return {};

  return GetThumbUrl(m_urls.front());
}

void CScraperUrl::GetThumbUrls(std::vector<std::string>& thumbs,
                               const std::string& type,
                               int season,
                               bool unique) const
{
  for (const auto& url : m_urls)
  {
    if (url.m_aspect == type || type.empty() || url.m_aspect.empty())
    {
      if ((url.m_type == CScraperUrl::UrlType::General && season == -1) ||
          (url.m_type == CScraperUrl::UrlType::Season && url.m_season == season))
      {
        std::string thumbUrl = GetThumbUrl(url);
        if (!unique || std::find(thumbs.begin(), thumbs.end(), thumbUrl) == thumbs.end())
          thumbs.push_back(thumbUrl);
      }
    }
  }
}

bool CScraperUrl::Parse()
{
  if (m_parsed)
    return true;

  auto dataToParse = m_data;
  m_data.clear();
  return ParseFromData(dataToParse);
}

bool CScraperUrl::ParseFromData(const std::string& data)
{
  if (data.empty())
    return false;

  CXBMCTinyXML doc;
  /* strUrl is coming from internal sources (usually generated by scraper or from database)
   * so strUrl is always in UTF-8 */
  doc.Parse(data, TIXML_ENCODING_UTF8);

  auto pElement = doc.RootElement();
  if (pElement == nullptr)
  {
    m_urls.emplace_back(data);
    m_data = data;
  }
  else
  {
    while (pElement != nullptr)
    {
      ParseAndAppendUrl(pElement);
      pElement = pElement->NextSiblingElement(pElement->Value());
    }
  }

  m_parsed = true;
  return true;
}

bool CScraperUrl::ParseAndAppendUrl(const TiXmlElement* element)
{
  if (element == nullptr || element->FirstChild() == nullptr ||
      element->FirstChild()->Value() == nullptr)
    return false;

  bool wasEmpty = m_data.empty();

  std::stringstream stream;
  stream << *element;
  m_data += stream.str();

  SUrlEntry url(element->FirstChild()->ValueStr());
  url.m_spoof = XMLUtils::GetAttribute(element, "spoof");

  const char* szPost = element->Attribute("post");
  if (szPost && StringUtils::CompareNoCase(szPost, "yes") == 0)
    url.m_post = true;
  else
    url.m_post = false;

  const char* szIsGz = element->Attribute("gzip");
  if (szIsGz && StringUtils::CompareNoCase(szIsGz, "yes") == 0)
    url.m_isgz = true;
  else
    url.m_isgz = false;

  url.m_cache = XMLUtils::GetAttribute(element, "cache");

  const char* szType = element->Attribute("type");
  if (szType && StringUtils::CompareNoCase(szType, "season") == 0)
  {
    url.m_type = UrlType::Season;
    const char* szSeason = element->Attribute("season");
    if (szSeason)
      url.m_season = atoi(szSeason);
  }

  url.m_aspect = XMLUtils::GetAttribute(element, "aspect");
  url.m_preview = XMLUtils::GetAttribute(element, "preview");

  m_urls.push_back(url);

  if (wasEmpty)
    m_parsed = true;

  return true;
}

// XML format is of strUrls is:
// <TAG><url>...</url>...</TAG> (parsed by ParseElement) or <url>...</url> (ditto)
bool CScraperUrl::ParseAndAppendUrlsFromEpisodeGuide(const std::string& episodeGuide)
{
  if (episodeGuide.empty())
    return false;

  // ok, now parse the xml file
  CXBMCTinyXML doc;
  /* strUrls is coming from internal sources so strUrls is always in UTF-8 */
  doc.Parse(episodeGuide, TIXML_ENCODING_UTF8);
  if (doc.RootElement() == nullptr)
    return false;

  bool wasEmpty = m_data.empty();

  TiXmlHandle docHandle(&doc);
  auto link = docHandle.FirstChild("episodeguide").Element();
  if (link->FirstChildElement("url"))
  {
    for (link = link->FirstChildElement("url"); link; link = link->NextSiblingElement("url"))
      ParseAndAppendUrl(link);
  }
  else if (link->FirstChild() && link->FirstChild()->Value())
    ParseAndAppendUrl(link);

  if (wasEmpty)
    m_parsed = true;

  return true;
}

void CScraperUrl::AddParsedUrl(const std::string& url,
                               const std::string& aspect,
                               const std::string& preview,
                               const std::string& referrer,
                               const std::string& cache,
                               bool post,
                               bool isgz,
                               int season)
{
  bool wasEmpty = m_data.empty();

  TiXmlElement thumb("thumb");
  thumb.SetAttribute("spoof", referrer);
  thumb.SetAttribute("cache", cache);
  if (post)
    thumb.SetAttribute("post", "yes");
  if (isgz)
    thumb.SetAttribute("gzip", "yes");
  if (season >= 0)
  {
    thumb.SetAttribute("season", std::to_string(season));
    thumb.SetAttribute("type", "season");
  }
  thumb.SetAttribute("aspect", aspect);
  thumb.SetAttribute("preview", preview);
  TiXmlText text(url);
  thumb.InsertEndChild(text);

  m_data << thumb;

  SUrlEntry nUrl(url);
  nUrl.m_spoof = referrer;
  nUrl.m_post = post;
  nUrl.m_isgz = isgz;
  nUrl.m_cache = cache;
  nUrl.m_preview = preview;
  if (season >= 0)
  {
    nUrl.m_type = UrlType::Season;
    nUrl.m_season = season;
  }
  nUrl.m_aspect = aspect;

  m_urls.push_back(nUrl);

  if (wasEmpty)
    m_parsed = true;
}

std::string CScraperUrl::GetThumbUrl(const CScraperUrl::SUrlEntry& entry)
{
  if (entry.m_spoof.empty())
    return entry.m_url;

  return entry.m_url + "|Referer=" + CURL::Encode(entry.m_spoof);
}

bool CScraperUrl::Get(const SUrlEntry& scrURL,
                      std::string& strHTML,
                      XFILE::CCurlFile& http,
                      const std::string& cacheContext)
{
  CURL url(scrURL.m_url);
  http.SetReferer(scrURL.m_spoof);
  std::string strCachePath;

  if (!scrURL.m_cache.empty())
  {
    strCachePath = URIUtils::AddFileToFolder(
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_cachePath, "scrapers",
        cacheContext, scrURL.m_cache);
    if (XFILE::CFile::Exists(strCachePath))
    {
      XFILE::CFile file;
      std::vector<uint8_t> buffer;
      if (file.LoadFile(strCachePath, buffer) > 0)
      {
        strHTML.assign(reinterpret_cast<char*>(buffer.data()), buffer.size());
        return true;
      }
    }
  }

  auto strHTML1 = strHTML;

  if (scrURL.m_post)
  {
    std::string strOptions = url.GetOptions();
    strOptions = strOptions.substr(1);
    url.SetOptions("");

    if (!http.Post(url.Get(), strOptions, strHTML1))
      return false;
  }
  else if (!http.Get(url.Get(), strHTML1))
    return false;

  strHTML = strHTML1;

  const auto mimeType = http.GetProperty(XFILE::FileProperty::MIME_TYPE);
  CMime::EFileType ftype = CMime::GetFileTypeFromMime(mimeType);
  if (ftype == CMime::FileTypeUnknown)
    ftype = CMime::GetFileTypeFromContent(strHTML);

  if (ftype == CMime::FileTypeZip || ftype == CMime::FileTypeGZip)
  {
    XFILE::CZipFile file;
    std::string strBuffer;
    auto iSize = file.UnpackFromMemory(
        strBuffer, strHTML, scrURL.m_isgz); // FIXME: use FileTypeGZip instead of scrURL.m_isgz?
    if (iSize > 0)
    {
      strHTML = strBuffer;
      CLog::Log(LOGDEBUG, "{}: Archive \"{}\" was unpacked in memory", __FUNCTION__, scrURL.m_url);
    }
    else
      CLog::Log(LOGWARNING, "{}: \"{}\" looks like archive but cannot be unpacked", __FUNCTION__,
                scrURL.m_url);
  }

  const auto reportedCharset = http.GetProperty(XFILE::FileProperty::CONTENT_CHARSET);
  if (ftype == CMime::FileTypeHtml)
  {
    std::string realHtmlCharset, converted;
    if (!CCharsetDetection::ConvertHtmlToUtf8(strHTML, converted, reportedCharset, realHtmlCharset))
      CLog::Log(LOGWARNING,
                "{}: Can't find precise charset for HTML \"{}\", using \"{}\" as fallback",
                __FUNCTION__, scrURL.m_url, realHtmlCharset);
    else
      CLog::Log(LOGDEBUG, "{}: Using \"{}\" charset for HTML \"{}\"", __FUNCTION__, realHtmlCharset,
                scrURL.m_url);

    strHTML = converted;
  }
  else if (ftype == CMime::FileTypeXml)
  {
    CXBMCTinyXML xmlDoc;
    xmlDoc.Parse(strHTML, reportedCharset);

    const auto realXmlCharset = xmlDoc.GetUsedCharset();
    if (!realXmlCharset.empty())
    {
      CLog::Log(LOGDEBUG, "{}: Using \"{}\" charset for XML \"{}\"", __FUNCTION__, realXmlCharset,
                scrURL.m_url);
      std::string converted;
      g_charsetConverter.ToUtf8(realXmlCharset, strHTML, converted);
      strHTML = converted;
    }
  }
  else if (ftype == CMime::FileTypePlainText ||
           StringUtils::EqualsNoCase(mimeType.substr(0, 5), "text/"))
  {
    std::string realTextCharset;
    std::string converted;
    CCharsetDetection::ConvertPlainTextToUtf8(strHTML, converted, reportedCharset, realTextCharset);
    strHTML = converted;
    if (reportedCharset != realTextCharset)
      CLog::Log(LOGWARNING,
                "{}: Using \"{}\" charset for plain text \"{}\" instead of server reported \"{}\" "
                "charset",
                __FUNCTION__, realTextCharset, scrURL.m_url, reportedCharset);
    else
      CLog::Log(LOGDEBUG, "{}: Using \"{}\" charset for plain text \"{}\"", __FUNCTION__,
                realTextCharset, scrURL.m_url);
  }
  else if (!reportedCharset.empty())
  {
    CLog::Log(LOGDEBUG, "{}: Using \"{}\" charset for \"{}\"", __FUNCTION__, reportedCharset,
              scrURL.m_url);
    if (reportedCharset != "UTF-8")
    {
      std::string converted;
      g_charsetConverter.ToUtf8(reportedCharset, strHTML, converted);
      strHTML = converted;
    }
  }
  else
    CLog::Log(LOGDEBUG, "{}: Using content of \"{}\" as binary or text with \"UTF-8\" charset",
              __FUNCTION__, scrURL.m_url);

  if (!scrURL.m_cache.empty())
  {
    const auto strCachePath = URIUtils::AddFileToFolder(
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_cachePath, "scrapers",
        cacheContext, scrURL.m_cache);
    XFILE::CFile file;
    if (!file.OpenForWrite(strCachePath, true) ||
        file.Write(strHTML.data(), strHTML.size()) != static_cast<ssize_t>(strHTML.size()))
      return false;
  }
  return true;
}
