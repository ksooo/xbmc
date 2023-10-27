/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DAVDirectory.h"

#include "CurlFile.h"
#include "DAVCommon.h"
#include "DAVFile.h"
#include "FileItem.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

using namespace XFILE;

CDAVDirectory::CDAVDirectory(void) = default;
CDAVDirectory::~CDAVDirectory(void) = default;

namespace
{
/*
 * Parses a <response>
 *
 * <!ELEMENT response (href, ((href*, status)|(propstat+)), responsedescription?) >
 * <!ELEMENT propstat (prop, status, responsedescription?) >
 *
 */
std::shared_ptr<CFileItem> ParseResponse(const CURL& url, const tinyxml2::XMLElement* element)
{
  std::string path;
  int size{-1};
  time_t datetime{-1};
  std::string label;
  bool isFolder{false};

  /* Iterate response children elements */
  for (auto* responseChild = element->FirstChildElement(); responseChild;
       responseChild = responseChild->NextSiblingElement())
  {
    if (CDAVCommon::ValueWithoutNamespace(responseChild, "href") && !responseChild->NoChildren())
    {
      path = responseChild->FirstChild()->Value();
      URIUtils::RemoveSlashAtEnd(path);
    }
    else if (CDAVCommon::ValueWithoutNamespace(responseChild, "propstat"))
    {
      if (CDAVCommon::GetStatusTag(responseChild->ToElement()).find("200 OK") != std::string::npos)
      {
        /* Iterate propstat children elements */
        for (auto* propstatChild = responseChild->FirstChild(); propstatChild;
             propstatChild = propstatChild->NextSibling())
        {
          if (CDAVCommon::ValueWithoutNamespace(propstatChild, "prop"))
          {
            /* Iterate all properties available */
            for (auto* propChild = propstatChild->FirstChildElement(); propChild;
                 propChild = propChild->NextSiblingElement())
            {
              if (CDAVCommon::ValueWithoutNamespace(propChild, "getcontentlength") &&
                  !propChild->NoChildren())
              {
                size = strtoll(propChild->FirstChild()->Value(), NULL, 10);
              }
              else if (CDAVCommon::ValueWithoutNamespace(propChild, "getlastmodified") &&
                       !propChild->NoChildren())
              {
                struct tm timeDate = {};
                strptime(propChild->FirstChild()->Value(), "%a, %d %b %Y %T", &timeDate);
                datetime = mktime(&timeDate);
              }
              else if (CDAVCommon::ValueWithoutNamespace(propChild, "displayname") &&
                       !propChild->NoChildren())
              {
                label = CURL::Decode(propChild->FirstChild()->Value());
              }
              else if (datetime >= 0 &&
                       CDAVCommon::ValueWithoutNamespace(propChild, "creationdate") &&
                       !propChild->NoChildren())
              {
                struct tm timeDate = {};
                strptime(propChild->FirstChild()->Value(), "%Y-%m-%dT%T", &timeDate);
                datetime = mktime(&timeDate);
              }
              else if (CDAVCommon::ValueWithoutNamespace(propChild, "resourcetype"))
              {
                if (CDAVCommon::ValueWithoutNamespace(propChild->FirstChild(), "collection"))
                {
                  isFolder = true;
                }
              }
            }
          }
        }
      }
    }
  }

  if (!path.empty())
  {
    if (!url.GetProtocolOptions().empty())
      path += "|" + url.GetProtocolOptions();

    auto item{std::make_shared<CFileItem>(path, isFolder)};
    item->SetLabel(label);
    item->m_dwSize = size;
    item->m_dateTime = datetime;
    return item;
  }

  CLog::LogF(LOGERROR, "Parse error");
  return {};
}
} // unnamed namespace

bool CDAVDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  CCurlFile dav;
  std::string strRequest = "PROPFIND";

  dav.SetCustomRequest(strRequest);
  dav.SetMimeType("text/xml; charset=\"utf-8\"");
  dav.SetRequestHeader("depth", 1);
  dav.SetPostData(
    "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
    " <D:propfind xmlns:D=\"DAV:\">"
    "   <D:prop>"
    "     <D:resourcetype/>"
    "     <D:getcontentlength/>"
    "     <D:getlastmodified/>"
    "     <D:creationdate/>"
    "     <D:displayname/>"
    "    </D:prop>"
    "  </D:propfind>");

  if (!dav.Open(url))
  {
    CLog::LogF(LOGERROR, "Unable to get dav directory ({})", url.GetRedacted());
    return false;
  }

  std::string strResponse;
  dav.ReadData(strResponse);

  std::string fileCharset(dav.GetProperty(XFILE::FILE_PROPERTY_CONTENT_CHARSET));
  CXBMCTinyXML2 davResponse;
  davResponse.Parse(strResponse);

  if (!davResponse.Parse(strResponse))
  {
    CLog::LogF(LOGERROR, "Unable to process dav directory ({})", url.GetRedacted());
    dav.Close();
    return false;
  }

  // Iterate over all responses
  for (auto* child = davResponse.RootElement()->FirstChild(); child; child = child->NextSibling())
  {
    if (CDAVCommon::ValueWithoutNamespace(child, "response"))
    {
      const auto item{ParseResponse(url, child->ToElement())};
      if (!item)
        continue;

      const CURL url2(item->GetPath());
      std::string itemPath(URIUtils::AddFileToFolder(url.GetWithoutFilename(), url2.GetFileName()));

      if (item->GetLabel().empty())
      {
        std::string name(itemPath);
        URIUtils::RemoveSlashAtEnd(name);
        item->SetLabel(CURL::Decode(URIUtils::GetFileName(name)));
      }

      if (item->m_bIsFolder)
        URIUtils::AddSlashAtEnd(itemPath);

      if (!item->IsURL(url))
        items.Add(item);
    }
  }

  dav.Close();

  return true;
}

bool CDAVDirectory::Create(const CURL& url)
{
  CDAVFile dav;
  std::string strRequest = "MKCOL";

  dav.SetCustomRequest(strRequest);

  if (!dav.Execute(url))
  {
    CLog::Log(LOGERROR, "{} - Unable to create dav directory ({}) - {}", __FUNCTION__,
              url.GetRedacted(), dav.GetLastResponseCode());
    return false;
  }

  dav.Close();

  return true;
}

bool CDAVDirectory::Exists(const CURL& url)
{
  CCurlFile dav;

  // Set the PROPFIND custom request else we may not find folders, depending
  // on the server's configuration
  std::string strRequest = "PROPFIND";
  dav.SetCustomRequest(strRequest);
  dav.SetRequestHeader("depth", 0);

  return dav.Exists(url);
}

bool CDAVDirectory::Remove(const CURL& url)
{
  CDAVFile dav;
  std::string strRequest = "DELETE";

  dav.SetCustomRequest(strRequest);

  if (!dav.Execute(url))
  {
    CLog::Log(LOGERROR, "{} - Unable to delete dav directory ({}) - {}", __FUNCTION__,
              url.GetRedacted(), dav.GetLastResponseCode());
    return false;
  }

  dav.Close();

  return true;
}
