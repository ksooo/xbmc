/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRTimerIntSettingDefinition.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_general.h"
#include "utils/log.h"

namespace PVR
{
std::vector<CPVRTimerIntSettingDefinition> CPVRTimerIntSettingDefinition::
    CreateSettingDefinitionsList(int clientId,
                                 unsigned int timerTypeId,
                                 struct PVR_INT_SETTING_DEFINITION** defs,
                                 unsigned int settingDefsSize)
{
  std::vector<CPVRTimerIntSettingDefinition> defsList;
  if (defs && settingDefsSize > 0)
  {
    defsList.reserve(settingDefsSize);
    for (unsigned int i = 0; i < settingDefsSize; ++i)
    {
      const PVR_INT_SETTING_DEFINITION* def{defs[i]};
      if (def)
        defsList.emplace_back(clientId, timerTypeId, *def);
    }
  }
  return defsList;
}

CPVRTimerIntSettingDefinition::CPVRTimerIntSettingDefinition(int clientId,
                                                             unsigned int timerTypeId,
                                                             const PVR_INT_SETTING_DEFINITION& def)
  : m_clientId(clientId),
    m_timerTypeId(timerTypeId),
    m_id(def.iId),
    m_name(def.strName ? def.strName : ""),
    m_values(def.values, def.iValuesSize, def.iDefaultValue),
    m_minValue(def.iMinValue),
    m_step(def.iStep),
    m_maxValue(def.iMaxValue),
    m_readonlyConditions(def.iReadonlyConditions)
{
}

bool CPVRTimerIntSettingDefinition::operator==(const CPVRTimerIntSettingDefinition& right) const
{
  return (m_id == right.m_id && m_name == right.m_name && m_values == right.m_values &&
          m_minValue == right.m_minValue && m_step == right.m_step &&
          m_maxValue == right.m_maxValue && m_readonlyConditions == right.m_readonlyConditions);
}

bool CPVRTimerIntSettingDefinition::operator!=(const CPVRTimerIntSettingDefinition& right) const
{
  return !(*this == right);
}

bool CPVRTimerIntSettingDefinition::IsReadonlyForTimerState(PVR_TIMER_STATE timerState) const
{
  switch (timerState)
  {
    case PVR_TIMER_STATE_DISABLED:
      return m_readonlyConditions & PVR_SETTING_READONLY_CONDITION_TIMER_DISABLED;
    case PVR_TIMER_STATE_SCHEDULED:
    case PVR_TIMER_STATE_CONFLICT_OK:
    case PVR_TIMER_STATE_CONFLICT_NOK:
      return m_readonlyConditions & PVR_SETTING_READONLY_CONDITION_TIMER_SCHEDULED;
    case PVR_TIMER_STATE_RECORDING:
      return m_readonlyConditions & PVR_SETTING_READONLY_CONDITION_TIMER_RECORDING;
    case PVR_TIMER_STATE_COMPLETED:
    case PVR_TIMER_STATE_ABORTED:
    case PVR_TIMER_STATE_CANCELLED:
    case PVR_TIMER_STATE_ERROR:
      return m_readonlyConditions & PVR_SETTING_READONLY_CONDITION_TIMER_COMPLETED;
    default:
      CLog::LogF(LOGWARNING, "Unhandled timer state {}", timerState);
      break;
  }
  return false;
}
} // namespace PVR
