/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <algorithm>

#include "PVRSettings.h"

#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/timers/PVRTimerType.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"

using namespace PVR;

static const int PRIORITY_MIN_VALUE = 1;
static const int PRIORITY_MAX_VALUE = 100;

static const int LIFETIME_MIN_VALUE = 1;
static const int LIFETIME_MAX_VALUE = 365;

struct SortTypeValues
{
  bool operator() (const std::pair<std::string, int> &lhs, const std::pair<std::string, int> &rhs)
  {
    return (lhs.second < rhs.second);
  }
};

bool CPVRSettings::IsSettingVisible(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == nullptr)
    return false;

  const std::string &settingId = setting->GetId();

  if ((settingId == CSettings::SETTING_PVRRECORD_DEFAULTPRIORITY) ||
      (settingId == CSettings::SETTING_PVRRECORD_DEFAULTLIFETIME) ||
      (settingId == CSettings::SETTING_PVRRECORD_PREVENTDUPLICATEEPISODES))
  {
    std::vector<CPVRTimerTypePtr> timerTypes(CPVRTimerType::GetAllTypes());
    std::vector< std::pair<std::string, int> > currValues;
    std::vector< std::pair<std::string, int> > prevValues;

    for (const auto &timerType : timerTypes)
    {
      if (!currValues.empty())
      {
        prevValues = currValues;
        currValues.clear();
      }

      if (settingId == CSettings::SETTING_PVRRECORD_DEFAULTPRIORITY)
        timerType->GetPriorityValues(currValues);
      else if (settingId == CSettings::SETTING_PVRRECORD_DEFAULTLIFETIME)
        timerType->GetLifetimeValues(currValues);
      else if (settingId == CSettings::SETTING_PVRRECORD_PREVENTDUPLICATEEPISODES)
        timerType->GetPreventDuplicateEpisodesValues(currValues);

      std::sort(currValues.begin(), currValues.end(), SortTypeValues());

      if (!currValues.empty() && !prevValues.empty() && prevValues != currValues)
      {
        // At least one type has different value definitions. Hide the setting.
        return false;
      }
    }

    // All types have the same value definitions. Show the setting.
    return true;
  }
  else
  {
    // Show all other settings unconditionally.
    return true;
  }
}

void CPVRSettings::PriorityFiller(
  const CSetting * /*setting*/, std::vector< std::pair<std::string, int> > &list, int & /*current*/, void * /*data*/)
{
  list.clear();

  const std::vector<CPVRTimerTypePtr> timerTypes(CPVRTimerType::GetAllTypes());
  for (const auto &timerType : timerTypes)
  {
    timerType->GetPriorityValues(list);
    if (!list.empty())
      break;
  }

  if (list.empty())
    GetPriorityDefaults(list);
}

void CPVRSettings::LifetimeFiller(
  const CSetting * /*setting*/, std::vector< std::pair<std::string, int> > &list, int & /*current*/, void * /*data*/)
{
  list.clear();

  const std::vector<CPVRTimerTypePtr> timerTypes(CPVRTimerType::GetAllTypes());
  for (const auto &timerType : timerTypes)
  {
    timerType->GetLifetimeValues(list);
    if (!list.empty())
      break;
  }

  if (list.empty())
    GetLifetimeDefaults(list);
}

void CPVRSettings::PreventDuplicatesFiller(
  const CSetting * /*setting*/, std::vector< std::pair<std::string, int> > &list, int & /*current*/, void * /*data*/)
{
  list.clear();

  const std::vector<CPVRTimerTypePtr> timerTypes(CPVRTimerType::GetAllTypes());
  for (const auto &timerType : timerTypes)
  {
    timerType->GetPreventDuplicateEpisodesValues(list);
    if (!list.empty())
      break;
  }

  if (list.empty())
    GetPreventDuplicatesDefaults(list);
}

int CPVRSettings::GetPriorityDefaults(std::vector< std::pair<std::string, int> > &list)
{
  list.clear();

  for (int i = PRIORITY_MIN_VALUE; i <= PRIORITY_MAX_VALUE; ++i)
    list.push_back(std::make_pair(StringUtils::Format("%d", i), i));

  return PRIORITY_DEFAULT_VALUE;
}

int CPVRSettings::GetLifetimeDefaults(std::vector< std::pair<std::string, int> > &list)
{
  list.clear();

  for (int i = LIFETIME_MIN_VALUE; i <= LIFETIME_MAX_VALUE; ++i)
    list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(17999).c_str(), i), i)); // "%s days"

  return LIFETIME_DEFAULT_VALUE;
}

int CPVRSettings::GetPreventDuplicatesDefaults(std::vector< std::pair<std::string, int> > &list)
{
  list.clear();

  list.push_back(std::make_pair(g_localizeStrings.Get(815), 0)); // "Record all episodes"
  list.push_back(std::make_pair(g_localizeStrings.Get(816), 1)); // "Record only new episodes"

  return PREVDUP_DEFAULT_VALUE;
}

void CPVRSettings::MarginTimeFiller(
  const CSetting * /*setting*/, std::vector< std::pair<std::string, int> > &list, int & /*current*/, void * /*data*/)
{
  list.clear();

  static const int marginTimeValues[] =
  {
    0, 1, 3, 5, 10, 15, 20, 30, 60, 90, 120, 180 // minutes
  };
  static const size_t marginTimeValuesCount = sizeof(marginTimeValues) / sizeof(int);

  for (size_t i = 0; i < marginTimeValuesCount; ++i)
  {
    int iValue = marginTimeValues[i];
    list.push_back(
      std::make_pair(StringUtils::Format(g_localizeStrings.Get(14044).c_str(), iValue) /* %i min */, iValue));
  }
}
