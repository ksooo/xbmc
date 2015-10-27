#pragma once
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

#include <string>
#include <utility>
#include <vector>

class CSetting;

namespace PVR
{
  class CPVRSettings
  {
  public:
  /**
    * Default values.
    *
    */
    static const int PRIORITY_DEFAULT_VALUE = 50;
    static const int LIFETIME_DEFAULT_VALUE = 99; // days
    static const int PREVDUP_DEFAULT_VALUE  = 0;  // record all

  /**
    * Dynamically hide or Show settings.
    *
    */
    static bool IsSettingVisible(const std::string &condition, const std::string &value, const CSetting *setting, void *data);

  /**
    * settings value filler for priority for PVR timers.
    *
    */
    static void PriorityFiller(
      const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

  /**
    * settings value filler for lifetime for PVR timers.
    *
    */
    static void LifetimeFiller(
      const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

  /**
    * settings value filler for duplicate prevention mode for PVR timers.
    *
    */
    static void PreventDuplicatesFiller(
      const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

  /**
    * settings value filler for start/end recording margin time for PVR timers.
    *
    */
    static void MarginTimeFiller(
      const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

  /**
    * settings value filler for default priority values for PVR timers.
    *
    */
    static int GetPriorityDefaults(std::vector< std::pair<std::string, int> > &list);

  /**
    * settings value filler for default lifetime values for PVR timers.
    *
    */
    static int GetLifetimeDefaults(std::vector< std::pair<std::string, int> > &list);

  /**
    * settings value filler for default duplicate prevention mode defaults for PVR timers.
    *
    */
    static int GetPreventDuplicatesDefaults(std::vector< std::pair<std::string, int> > &list);

  private:
    CPVRSettings() = delete;
    CPVRSettings(const CPVRSettings&) = delete;
    CPVRSettings& operator=(CPVRSettings const&) = delete;
  };
}
