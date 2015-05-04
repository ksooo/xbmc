/*
 *      Copyright (C) 2012-2015 Team Kodi
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

#include "GUIDialogPVRTimerWeekdaysSettings.h"

#include "addons/include/xbmc_pvr_types.h"
#include "settings/lib/Setting.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/log.h"

using namespace PVR;

#define SETTING_TMR_WDS_MONDAY    "timerweekdays.monday"
#define SETTING_TMR_WDS_TUESDAY   "timerweekdays.tuesday"
#define SETTING_TMR_WDS_WEDNESDAY "timerweekdays.wednesday"
#define SETTING_TMR_WDS_THURSDAY  "timerweekdays.thursday"
#define SETTING_TMR_WDS_FRIDAY    "timerweekdays.friday"
#define SETTING_TMR_WDS_SATURDAY  "timerweekdays.saturday"
#define SETTING_TMR_WDS_SUNDAY    "timerweekdays.sunday"

CGUIDialogPVRTimerWeekdaysSettings::CGUIDialogPVRTimerWeekdaysSettings() :
  CGUIDialogSettingsManualBase(WINDOW_DIALOG_PVR_TIMER_WEEKDAYS_SETTING, "DialogPVRTimerWeekdaysSettings.xml"),
  m_iWeekdays(PVR_WEEKDAY_ALLDAYS)
{
  m_loadType = LOAD_EVERY_TIME;
}

// virtual
CGUIDialogPVRTimerWeekdaysSettings::~CGUIDialogPVRTimerWeekdaysSettings()
{
}

// virtual
void CGUIDialogPVRTimerWeekdaysSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  UpdateControlsState();
}

// virtual
void CGUIDialogPVRTimerWeekdaysSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("pvrtimerweekdayssettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerWeekdaysSettings::InitializeSettings - Unable to add settings category");
    return;
  }

  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerWeekdaysSettings::InitializeSettings - Unable to add settings group");
    return;
  }

  AddToggle(group, SETTING_TMR_WDS_MONDAY,    831, 0, !!(m_iWeekdays & PVR_WEEKDAY_MONDAY));
  AddToggle(group, SETTING_TMR_WDS_TUESDAY,   832, 0, !!(m_iWeekdays & PVR_WEEKDAY_TUESDAY));
  AddToggle(group, SETTING_TMR_WDS_WEDNESDAY, 833, 0, !!(m_iWeekdays & PVR_WEEKDAY_WEDNESDAY));
  AddToggle(group, SETTING_TMR_WDS_THURSDAY,  834, 0, !!(m_iWeekdays & PVR_WEEKDAY_THURSDAY));
  AddToggle(group, SETTING_TMR_WDS_FRIDAY,    835, 0, !!(m_iWeekdays & PVR_WEEKDAY_FRIDAY));
  AddToggle(group, SETTING_TMR_WDS_SATURDAY,  836, 0, !!(m_iWeekdays & PVR_WEEKDAY_SATURDAY));
  AddToggle(group, SETTING_TMR_WDS_SUNDAY,    837, 0, !!(m_iWeekdays & PVR_WEEKDAY_SUNDAY));
}

// virtual
void CGUIDialogPVRTimerWeekdaysSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerWeekdaysSettings::OnSettingChanged - No setting");
    return;
  }

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  bool bSet = dynamic_cast<const CSettingBool*>(setting)->GetValue();

  const std::string &settingId = setting->GetId();

  if (settingId == SETTING_TMR_WDS_MONDAY)
  {
    m_iWeekdays = bSet ? m_iWeekdays | PVR_WEEKDAY_MONDAY : m_iWeekdays & ~PVR_WEEKDAY_MONDAY;
  }
  else if (settingId == SETTING_TMR_WDS_TUESDAY)
  {
    m_iWeekdays = bSet ? m_iWeekdays | PVR_WEEKDAY_TUESDAY : m_iWeekdays & ~PVR_WEEKDAY_TUESDAY;
  }
  else if (settingId == SETTING_TMR_WDS_WEDNESDAY)
  {
    m_iWeekdays = bSet ? m_iWeekdays | PVR_WEEKDAY_WEDNESDAY : m_iWeekdays & ~PVR_WEEKDAY_WEDNESDAY;
  }
  else if (settingId == SETTING_TMR_WDS_THURSDAY)
  {
    m_iWeekdays = bSet ? m_iWeekdays | PVR_WEEKDAY_THURSDAY : m_iWeekdays & ~PVR_WEEKDAY_THURSDAY;
  }
  else if (settingId == SETTING_TMR_WDS_FRIDAY)
  {
    m_iWeekdays = bSet ? m_iWeekdays | PVR_WEEKDAY_FRIDAY : m_iWeekdays & ~PVR_WEEKDAY_FRIDAY;
  }
  else if (settingId == SETTING_TMR_WDS_SATURDAY)
  {
    m_iWeekdays = bSet ? m_iWeekdays | PVR_WEEKDAY_SATURDAY : m_iWeekdays & ~PVR_WEEKDAY_SATURDAY;
  }
  else if (settingId == SETTING_TMR_WDS_SUNDAY)
  {
    m_iWeekdays = bSet ? m_iWeekdays | PVR_WEEKDAY_SUNDAY : m_iWeekdays & ~PVR_WEEKDAY_SUNDAY;
  }

  UpdateControlsState();
}

// virtual
bool CGUIDialogPVRTimerWeekdaysSettings::AllowResettingSettings() const
{
  return false;
}

// virtual
void CGUIDialogPVRTimerWeekdaysSettings::Save()
{
}

void CGUIDialogPVRTimerWeekdaysSettings::UpdateControlsState()
{
  // If only one day is selected, disable the respective control to prevent deselection of the last remaining day.
  CONTROL_ENABLE_ON_CONDITION(
    GetSettingControl(SETTING_TMR_WDS_MONDAY)->GetID(),    !(m_iWeekdays == PVR_WEEKDAY_MONDAY));
  CONTROL_ENABLE_ON_CONDITION(
    GetSettingControl(SETTING_TMR_WDS_TUESDAY)->GetID(),   !(m_iWeekdays == PVR_WEEKDAY_TUESDAY));
  CONTROL_ENABLE_ON_CONDITION(
    GetSettingControl(SETTING_TMR_WDS_WEDNESDAY)->GetID(), !(m_iWeekdays == PVR_WEEKDAY_WEDNESDAY));
  CONTROL_ENABLE_ON_CONDITION(
    GetSettingControl(SETTING_TMR_WDS_THURSDAY)->GetID(),  !(m_iWeekdays == PVR_WEEKDAY_THURSDAY));
  CONTROL_ENABLE_ON_CONDITION(
    GetSettingControl(SETTING_TMR_WDS_FRIDAY)->GetID(),    !(m_iWeekdays == PVR_WEEKDAY_FRIDAY));
  CONTROL_ENABLE_ON_CONDITION(
    GetSettingControl(SETTING_TMR_WDS_SATURDAY)->GetID(),  !(m_iWeekdays == PVR_WEEKDAY_SATURDAY));
  CONTROL_ENABLE_ON_CONDITION(
    GetSettingControl(SETTING_TMR_WDS_SUNDAY)->GetID(),    !(m_iWeekdays == PVR_WEEKDAY_SUNDAY));
}
