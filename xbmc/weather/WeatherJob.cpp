/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WeatherJob.h"

#include "GUIUserMessages.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "network/Network.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/POUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

namespace
{
constexpr unsigned int LOCALIZED_TOKEN_FIRSTID = 370;
constexpr unsigned int LOCALIZED_TOKEN_LASTID = 395;
constexpr unsigned int LOCALIZED_TOKEN_FIRSTID2 = 1350;
constexpr unsigned int LOCALIZED_TOKEN_LASTID2 = 1449;
constexpr unsigned int LOCALIZED_TOKEN_FIRSTID3 = 11;
constexpr unsigned int LOCALIZED_TOKEN_LASTID3 = 17;
constexpr unsigned int LOCALIZED_TOKEN_FIRSTID4 = 71;
constexpr unsigned int LOCALIZED_TOKEN_LASTID4 = 97;

} // unnamed namespace

using namespace ADDON;
using namespace std::chrono_literals;

CWeatherJob::CWeatherJob(int location) : m_location(location)
{
}

bool CWeatherJob::DoWork()
{
  // wait for the network
  if (!CServiceBroker::GetNetwork().IsAvailable())
    return false;

  AddonPtr addon;
  if (!CServiceBroker::GetAddonMgr().GetAddon(
          CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
              CSettings::SETTING_WEATHER_ADDON),
          addon, AddonType::SCRIPT_WEATHER, OnlyEnabled::CHOICE_YES))
    return false;

  // initialize our sys.argv variables
  std::vector<std::string> argv;
  argv.emplace_back(addon->LibPath());

  const std::string strSetting{std::to_string(m_location)};
  argv.emplace_back(strSetting);

  // Download our weather
  CLog::Log(LOGINFO, "WEATHER: Downloading weather");

  // call our script, passing the areacode
  int scriptId = -1;
  if ((scriptId = CScriptInvocationManager::GetInstance().ExecuteAsync(argv[0], addon, argv)) >= 0)
  {
    while (true)
    {
      if (!CScriptInvocationManager::GetInstance().IsRunning(scriptId))
        break;

      KODI::TIME::Sleep(100ms);
    }

    CLog::Log(LOGINFO, "WEATHER: Successfully downloaded weather");

    SetFromProperties();

    // and send a message that we're done
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_WEATHER_FETCHED);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
  }
  else
    CLog::Log(LOGERROR, "WEATHER: Weather download failed!");

  return true;
}

const WeatherInfo& CWeatherJob::GetInfo() const
{
  return m_info;
}

const CWeatherManager::WeatherInfoV2& CWeatherJob::GetInfoV2() const
{
  return m_infoV2;
}

void CWeatherJob::LocalizeOverviewToken(std::string& token)
{
  // This routine is case-insensitive.
  std::string strLocStr;
  if (!token.empty())
  {
    const auto i{m_localizedTokens.find(token)};
    if (i != m_localizedTokens.cend())
      strLocStr = g_localizeStrings.Get(i->second);
  }

  if (strLocStr.empty())
    strLocStr = token; //if not found, let fallback

  token = strLocStr;
}

void CWeatherJob::LocalizeOverview(std::string& str)
{
  std::vector<std::string> words{StringUtils::Split(str, " ")};
  for (std::string& word : words)
    LocalizeOverviewToken(word);

  str = StringUtils::Join(words, " ");
}

void CWeatherJob::LoadLocalizedToken()
{
  // We load the english strings in to get our tokens
  std::string language = LANGUAGE_DEFAULT;
  const std::shared_ptr<const CSettingString> languageSetting{
      std::static_pointer_cast<const CSettingString>(
          CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
              CSettings::SETTING_LOCALE_LANGUAGE))};
  if (languageSetting)
    language = languageSetting->GetDefault();

  // Load the strings.po file
  CPODocument PODoc;
  if (PODoc.LoadFile(URIUtils::AddFileToFolder(CLangInfo::GetLanguagePath(language), "strings.po")))
  {
    int counter = 0;

    while (PODoc.GetNextEntry())
    {
      if (PODoc.GetEntryType() != ID_FOUND)
        continue;

      const uint32_t id{PODoc.GetEntryID()};
      PODoc.ParseEntry(ISSOURCELANG);

      if (id > LOCALIZED_TOKEN_LASTID2)
        break;

      if ((LOCALIZED_TOKEN_FIRSTID <= id && id <= LOCALIZED_TOKEN_LASTID) ||
          (LOCALIZED_TOKEN_FIRSTID2 <= id && id <= LOCALIZED_TOKEN_LASTID2) ||
          (LOCALIZED_TOKEN_FIRSTID3 <= id && id <= LOCALIZED_TOKEN_LASTID3) ||
          (LOCALIZED_TOKEN_FIRSTID4 <= id && id <= LOCALIZED_TOKEN_LASTID4))
      {
        if (!PODoc.GetMsgid().empty())
        {
          m_localizedTokens.try_emplace(PODoc.GetMsgid(), id);
          counter++;
        }
      }
    }

    CLog::LogF(LOGDEBUG, "POParser: loaded {} weather tokens", counter);
    return;
  }
}

namespace
{
std::string ConstructPath(const std::string& in)
{
  if (in.find('/') != std::string::npos || in.find('\\') != std::string::npos)
    return in;

  return URIUtils::AddFileToFolder(ICON_ADDON_PATH, (in.empty() || in == "N/A") ? "na.png" : in);
}

void FormatTemperature(std::string& text, double temp)
{
  const CTemperature temperature{CTemperature::CreateFromCelsius(temp)};
  text = StringUtils::Format("{:.0f}", temperature.To(g_langInfo.GetTemperatureUnit()));
}

std::string FormatTemperatureV2(const std::string& temperature)
{
  if (temperature.empty())
    return {};

  // Convert from celsius to system temperature unit and append resp. unit of measurement.
  const CTemperature t{CTemperature::CreateFromCelsius(std::strtod(temperature.c_str(), nullptr))};
  return StringUtils::Format("{:.0f}{}", t.To(g_langInfo.GetTemperatureUnit()),
                             g_langInfo.GetTemperatureUnitString());
}

std::string FormatWindV2(const std::string& direction, const std::string& wind)
{
  if (direction == "CALM")
  {
    return g_localizeStrings.Get(1410); // Calm
  }
  else
  {
    if (wind.empty())
      return {};

    const CSpeed speed{CSpeed::CreateFromKilometresPerHour(std::strtod(wind.c_str(), nullptr))};
    if (direction.empty())
    {
      return StringUtils::Format("{} {}", speed.To(g_langInfo.GetSpeedUnit()),
                                 g_langInfo.GetSpeedUnitString());
    }
    else
    {
      return StringUtils::Format(g_localizeStrings.Get(434), // From {direction} at {speed} {unit}
                                 direction, static_cast<int>(speed.To(g_langInfo.GetSpeedUnit())),
                                 g_langInfo.GetSpeedUnitString());
    }
  }
}

std::string FormatPercentageV2(const std::string& percentage)
{
  if (percentage.empty())
    return {};

  if (StringUtils::EndsWith(percentage, "%")) // Some add-ons return value including unit
    return percentage;

  return StringUtils::Format("{}%", percentage);
}
} // unnamed namespace

void CWeatherJob::SetPropertyV2(CGUIWindow* window, const std::string& prop)
{
  m_infoV2.try_emplace(prop, window->GetProperty(prop).asString());
}

void CWeatherJob::SetFromProperties()
{
  // Load in our tokens if necessary
  if (m_localizedTokens.empty())
    LoadLocalizedToken();

  CGUIWindow* window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_WEATHER);
  if (window)
  {
    std::string prop;
    std::string str;

    const CDateTime time{CDateTime::GetCurrentDateTime()};
    m_info.lastUpdateTime = time.GetAsLocalizedDateTime(false, false);

    prop = "Current.Condition";
    m_info.currentConditions = window->GetProperty(prop).asString();
    LocalizeOverview(m_info.currentConditions);
    m_infoV2.try_emplace(prop, m_info.currentConditions);

    prop = "Current.OutlookIcon";
    m_info.currentIcon = ConstructPath(window->GetProperty(prop).asString());
    m_infoV2.try_emplace(prop, m_info.currentIcon);
    m_infoV2.try_emplace("Current.ConditionIcon", m_info.currentIcon); // See spec.

    SetPropertyV2(window, "Current.FanartCode");

    prop = "Current.Temperature";
    str = window->GetProperty(prop).asString();
    FormatTemperature(m_info.currentTemperature, std::strtod(str.c_str(), nullptr));
    m_infoV2.try_emplace(prop, FormatTemperatureV2(str));

    prop = "Current.FeelsLike";
    str = window->GetProperty(prop).asString();
    FormatTemperature(m_info.currentFeelsLike, std::strtod(str.c_str(), nullptr));
    m_infoV2.try_emplace(prop, FormatTemperatureV2(str));

    prop = "Current.UVIndex";
    m_info.currentUVIndex = window->GetProperty(prop).asString();
    LocalizeOverview(m_info.currentUVIndex);
    m_infoV2.try_emplace(prop, m_info.currentUVIndex);

    prop = "Current.Wind";
    str = window->GetProperty(prop).asString();
    const CSpeed speed{CSpeed::CreateFromKilometresPerHour(std::strtod(str.c_str(), nullptr))};
    std::string direction = window->GetProperty("Current.WindDirection").asString();
    LocalizeOverviewToken(direction);
    if (direction == "CALM")
    {
      m_info.currentWind = g_localizeStrings.Get(1410);
    }
    else
    {
      m_info.currentWind = StringUtils::Format(
          g_localizeStrings.Get(434), direction,
          static_cast<int>(speed.To(g_langInfo.GetSpeedUnit())), g_langInfo.GetSpeedUnitString());
    }
    m_infoV2.try_emplace(prop, FormatWindV2(direction, str));

    prop = "Current.WindSpeed";
    const std::string windspeed{
        StringUtils::Format("{} {}", static_cast<int>(speed.To(g_langInfo.GetSpeedUnit())),
                            g_langInfo.GetSpeedUnitString())};
    window->SetProperty(prop, windspeed);
    m_infoV2.try_emplace(prop, windspeed);

    prop = "Current.DewPoint";
    str = window->GetProperty(prop).asString();
    FormatTemperature(m_info.currentDewPoint, std::strtod(str.c_str(), nullptr));
    m_infoV2.try_emplace(prop, FormatTemperatureV2(str));

    prop = "Current.Humidity";
    str = window->GetProperty(prop).asString();
    if (str.empty())
      m_info.currentHumidity.clear();
    else
      m_info.currentHumidity = StringUtils::Format("{}%", str);
    m_infoV2.try_emplace(prop, FormatPercentageV2(str));

    prop = "Current.Location";
    m_info.location = window->GetProperty(prop).asString();
    m_infoV2.try_emplace(prop, m_info.location);

    for (unsigned int i = 0; i < WeatherInfo::NUM_DAYS; ++i)
    {
      std::string strDay = StringUtils::Format("Day{}.Title", i);
      m_info.forecast[i].m_day = window->GetProperty(strDay).asString();
      LocalizeOverviewToken(m_info.forecast[i].m_day);
      m_infoV2.try_emplace(strDay, m_info.forecast[i].m_day);

      strDay = StringUtils::Format("Day{}.HighTemp", i);
      std::string str{window->GetProperty(strDay).asString()};
      FormatTemperature(m_info.forecast[i].m_high, std::strtod(str.c_str(), nullptr));
      m_infoV2.try_emplace(strDay, FormatTemperatureV2(str));

      strDay = StringUtils::Format("Day{}.LowTemp", i);
      str = window->GetProperty(strDay).asString();
      FormatTemperature(m_info.forecast[i].m_low, std::strtod(str.c_str(), nullptr));
      m_infoV2.try_emplace(strDay, FormatTemperatureV2(str));

      strDay = StringUtils::Format("Day{}.OutlookIcon", i);
      m_info.forecast[i].m_icon = ConstructPath(window->GetProperty(strDay).asString());
      m_infoV2.try_emplace(strDay, m_info.forecast[i].m_icon);

      strDay = StringUtils::Format("Day{}.Outlook", i);
      m_info.forecast[i].m_overview = window->GetProperty(strDay).asString();
      LocalizeOverview(m_info.forecast[i].m_overview);
      m_infoV2.try_emplace(strDay, m_info.forecast[i].m_overview);

      SetPropertyV2(window, StringUtils::Format("Day{}.FanartCode", i));
    }

    prop = "Current.Precipitation";
    str = window->GetProperty(prop).asString();
    m_infoV2.try_emplace(prop, FormatPercentageV2(str));

    prop = "Current.Cloudiness";
    str = window->GetProperty(prop).asString();
    m_infoV2.try_emplace(prop, FormatPercentageV2(str));

    int idx{1};
    while (true)
    {
      prop = StringUtils::Format("Hourly.{}.Time", idx);
      str = window->GetProperty(prop).asString();
      if (str.empty())
        break; // no time value, assume no data for this index

      m_infoV2.try_emplace(prop, str);

      SetPropertyV2(window, StringUtils::Format("Hourly.{}.LongDate", idx));
      SetPropertyV2(window, StringUtils::Format("Hourly.{}.ShortDate", idx));

      prop = StringUtils::Format("Hourly.{}.Outlook", idx);
      str = window->GetProperty(prop).asString();
      LocalizeOverview(str);
      m_infoV2.try_emplace(prop, str);

      SetPropertyV2(window, StringUtils::Format("Hourly.{}.OutlookIcon", idx));

      prop = StringUtils::Format("Hourly.{}.WindSpeed", idx);
      str = window->GetProperty(prop).asString();
      const CSpeed speed{CSpeed::CreateFromKilometresPerHour(std::strtod(str.c_str(), nullptr))};
      std::string direction{
          window->GetProperty(StringUtils::Format("Hourly.{}.WindDirection", idx)).asString()};
      LocalizeOverviewToken(direction);
      m_infoV2.try_emplace(prop, FormatWindV2(direction, str));

      prop = StringUtils::Format("Hourly.{}.Humidity", idx);
      str = window->GetProperty(prop).asString();
      m_infoV2.try_emplace(prop, FormatPercentageV2(str));

      prop = StringUtils::Format("Hourly.{}.Temperature", idx);
      str = window->GetProperty(prop).asString();
      m_infoV2.try_emplace(prop, FormatTemperatureV2(str));

      prop = StringUtils::Format("Hourly.{}.DewPoint", idx);
      str = window->GetProperty(prop).asString();
      m_infoV2.try_emplace(prop, FormatTemperatureV2(str));

      prop = StringUtils::Format("Hourly.{}.FeelsLike", idx);
      str = window->GetProperty(prop).asString();
      m_infoV2.try_emplace(prop, FormatTemperatureV2(str));

      prop = StringUtils::Format("Hourly.{}.Precipitation", idx);
      str = window->GetProperty(prop).asString();
      m_infoV2.try_emplace(prop, FormatPercentageV2(str));

      SetPropertyV2(window, StringUtils::Format("Hourly.{}.Pressure", idx));
      SetPropertyV2(window, StringUtils::Format("Hourly.{}.FanartCode", idx));

      idx++;
    }

    idx = 1;
    while (true)
    {
      prop = StringUtils::Format("Daily.{}.ShortDate", idx);
      str = window->GetProperty(prop).asString();
      if (str.empty())
        break; // no date value, assume no data for this index

      m_infoV2.try_emplace(prop, str);

      SetPropertyV2(window, StringUtils::Format("Daily.{}.ShortDay", idx));

      prop = StringUtils::Format("Daily.{}.HighTemperature", idx);
      str = window->GetProperty(prop).asString();
      m_infoV2.try_emplace(prop, FormatTemperatureV2(str));

      prop = StringUtils::Format("Daily.{}.LowTemperature", idx);
      str = window->GetProperty(prop).asString();
      m_infoV2.try_emplace(prop, FormatTemperatureV2(str));

      prop = StringUtils::Format("Daily.{}.Outlook", idx);
      str = window->GetProperty(prop).asString();
      LocalizeOverview(str);
      m_infoV2.try_emplace(prop, str);

      SetPropertyV2(window, StringUtils::Format("Daily.{}.OutlookIcon", idx));

      prop = StringUtils::Format("Daily.{}.WindSpeed", idx);
      str = window->GetProperty(prop).asString();
      const CSpeed speed{CSpeed::CreateFromKilometresPerHour(std::strtod(str.c_str(), nullptr))};
      std::string direction{
          window->GetProperty(StringUtils::Format("Daily.{}.WindDirection", idx)).asString()};
      LocalizeOverviewToken(direction);
      m_infoV2.try_emplace(prop, FormatWindV2(direction, str));

      prop = StringUtils::Format("Daily.{}.Precipitation", idx);
      str = window->GetProperty(prop).asString();
      m_infoV2.try_emplace(prop, FormatPercentageV2(str));

      SetPropertyV2(window, StringUtils::Format("Daily.{}.FanartCode", idx));

      idx++;
    }

    SetPropertyV2(window, "Locations");

    const int locations{std::atoi(str.c_str())};
    for (int i = 1; i <= locations; ++i)
    {
      SetPropertyV2(window, StringUtils::Format("Location{}", i));
    }
  }
}
