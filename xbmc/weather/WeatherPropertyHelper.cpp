/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WeatherPropertyHelper.h"

#include "LangInfo.h"
#include "guilib/GUIWindow.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "weather/WeatherTokenLocalizer.h"

#include <functional>
#include <map>
#include <string_view>

namespace
{
std::string FormatTemperature(const std::string& temperature)
{
  if (temperature.empty())
    return temperature;

  // Convert from celsius to system temperature unit and append resp. unit of measurement.
  const CTemperature t{CTemperature::CreateFromCelsius(std::strtod(temperature.c_str(), nullptr))};
  if (!t.IsValid())
    return temperature;

  return StringUtils::Format("{:.0f}{}", t.To(g_langInfo.GetTemperatureUnit()),
                             g_langInfo.GetTemperatureUnitString());
}

std::string FormatWindSpeed(const std::string& speed)
{
  if (speed.empty())
    return speed;

  // Convert from kilomters per hour to system speed unit and append resp. unit of measurement.
  const CSpeed s{CSpeed::CreateFromKilometresPerHour(std::strtod(speed.c_str(), nullptr))};
  if (!s.IsValid())
    return speed;

  return StringUtils::Format("{} {}", static_cast<int>(s.To(g_langInfo.GetSpeedUnit())),
                             g_langInfo.GetSpeedUnitString());
}

std::string FormatPercentage(const std::string& percentage)
{
  if (percentage.empty())
    return {};

  if (StringUtils::EndsWith(percentage, "%")) // Some add-ons return value including unit
    return percentage;

  return StringUtils::Format("{}%", percentage);
}

enum class L10nType
{
  NONE,
  OVERVIEW,
  OVERVIEW_TOKEN,
};

struct PropertyDetails
{
  std::function<std::string(std::string&)> formatter;
  L10nType l10n{L10nType::NONE};
};

// clang-format off
const std::map<std::string_view, PropertyDetails> propertyDetails{{
    {"Temperature",     {FormatTemperature, L10nType::NONE}},
    {"FeelsLike",       {FormatTemperature, L10nType::NONE}},
    {"DewPoint",        {FormatTemperature, L10nType::NONE}},
    {"HighTemp",        {FormatTemperature, L10nType::NONE}},
    {"LowTemp",         {FormatTemperature, L10nType::NONE}},
    {"HighTemperature", {FormatTemperature, L10nType::NONE}},
    {"LowTemperature",  {FormatTemperature, L10nType::NONE}},
    {"WindSpeed",       {FormatWindSpeed,   L10nType::NONE}},
    {"Humidity",        {FormatPercentage,  L10nType::NONE}},
    {"Precipitation",   {FormatPercentage,  L10nType::NONE}},
    {"Cloudiness",      {FormatPercentage,  L10nType::NONE}},
    {"Condition",       {{},                L10nType::OVERVIEW}},
    {"UVIndex",         {{},                L10nType::OVERVIEW}},
    {"Outlook",         {{},                L10nType::OVERVIEW}},
    {"WindDirection",   {{},                L10nType::OVERVIEW_TOKEN}},
    {"Title",           {{},                L10nType::OVERVIEW_TOKEN}},
}};
// clang-format on
} // unnamed namespace

CWeatherPropertyHelper::Property CWeatherPropertyHelper::GetProperty(const std::string& prop) const
{
  if (prop.empty())
    return {};

  std::string val{m_window.GetProperty(prop).asString()};

  const size_t pos{prop.find_last_of(".")};
  if (pos != std::string::npos && pos < (prop.size() - 1))
  {
    const auto& it{propertyDetails.find(prop.substr(pos + 1))};
    if (it != propertyDetails.cend())
    {
      const auto& [formatter, l10n] = (*it).second;

      // Need to format the prop?
      if (formatter)
        val = formatter(val);

      // Need to localize the prop?
      if (l10n == L10nType::OVERVIEW)
        val = m_localizer.LocalizeOverview(val);
      else if (l10n == L10nType::OVERVIEW_TOKEN)
        val = m_localizer.LocalizeOverviewToken(val);
    }
  }
  return {prop, val};
}

CWeatherPropertyHelper::Property CWeatherPropertyHelper::GetDayProperty(
    int index, const std::string& prop) const
{
  // Note: No '.' after 'Day'. This is different from 'Daily' and 'Hourly'.
  return GetProperty(StringUtils::Format("Day{}.{}", index, prop));
}

CWeatherPropertyHelper::Property CWeatherPropertyHelper::GetDailyProperty(
    int index, const std::string& prop) const
{
  return GetProperty(StringUtils::Format("Daily.{}.{}", index, prop));
}

CWeatherPropertyHelper::Property CWeatherPropertyHelper::GetHourlyProperty(
    int index, const std::string& prop) const
{
  return GetProperty(StringUtils::Format("Hourly.{}.{}", index, prop));
}

bool CWeatherPropertyHelper::HasDailyProperties(int index) const
{
  // No date value, assume no data for this index.
  return !m_window.GetProperty(StringUtils::Format("Daily.{}.ShortDate", index)).asString().empty();
}

bool CWeatherPropertyHelper::HasHourlyProperties(int index) const
{
  // No time value, assume no data for this index.
  return !m_window.GetProperty(StringUtils::Format("Hourly.{}.Time", index)).asString().empty();
}

std::string CWeatherPropertyHelper::FormatWind(const std::string& direction, const CSpeed& speed)
{
  if (direction == "CALM")
  {
    return g_localizeStrings.Get(1410); // Calm
  }
  else
  {
    if (!speed.IsValid())
      return {};

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

std::string CWeatherPropertyHelper::FormatFanartCode(const std::string& fanartCode,
                                                     const std::string& outlookIcon)
{
  if (!fanartCode.empty())
    return fanartCode; // take what we have

  if (!outlookIcon.empty())
  {
    // As fallback, extract code from outlook icon file name
    std::string code{URIUtils::GetFileName(outlookIcon)};
    URIUtils::RemoveExtension(code);
    return code;
  }

  return "na";
}
