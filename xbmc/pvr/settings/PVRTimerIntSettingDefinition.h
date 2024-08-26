/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_general.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_timers.h"
#include "pvr/PVRConstants.h" // PVR_CLIENT_INVALID_UID
#include "pvr/settings/PVRIntSettingValues.h"

#include <cstdint>
#include <string>
#include <vector>

namespace PVR
{
class CPVRTimerIntSettingDefinition
{
public:
  static std::vector<CPVRTimerIntSettingDefinition> CreateSettingDefinitionsList(
      int clientId,
      unsigned int timerTypeId,
      struct PVR_INT_SETTING_DEFINITION** defs,
      unsigned int settingDefsSize);

  CPVRTimerIntSettingDefinition(int clientId,
                                unsigned int timerTypeId,
                                const PVR_INT_SETTING_DEFINITION& def);

  virtual ~CPVRTimerIntSettingDefinition() = default;

  bool operator==(const CPVRTimerIntSettingDefinition& right) const;
  bool operator!=(const CPVRTimerIntSettingDefinition& right) const;

  int GetClientId() const { return m_clientId; }
  unsigned int GetTimerTypeId() const { return m_timerTypeId; }
  unsigned int GetId() const { return m_id; }
  const std::string& GetName() const { return m_name; }
  const std::vector<SettingIntValue>& GetValues() const { return m_values.GetValues(); }
  int GetDefaultValue() const { return m_values.GetDefaultValue(); }
  int GetMinValue() const { return m_minValue; }
  int GetStepValue() const { return m_step; }
  int GetMaxValue() const { return m_maxValue; }
  bool IsReadonlyForTimerState(PVR_TIMER_STATE timerState) const;

private:
  int m_clientId{PVR_CLIENT_INVALID_UID};
  unsigned int m_timerTypeId{PVR_TIMER_TYPE_NONE};
  unsigned int m_id{0};
  std::string m_name;
  CPVRIntSettingValues m_values;
  int m_minValue{0};
  int m_step{0};
  int m_maxValue{0};
  uint64_t m_readonlyConditions{PVR_SETTING_READONLY_CONDITION_NONE};
};
} // namespace PVR
