#pragma once
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

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

namespace PVR
{
  class CGUIDialogPVRTimerWeekdaysSettings : public CGUIDialogSettingsManualBase
  {
  public:
    CGUIDialogPVRTimerWeekdaysSettings();
    virtual ~CGUIDialogPVRTimerWeekdaysSettings();

    void SetWeekdays(unsigned int iWeekdays) { m_iWeekdays = iWeekdays; }
    unsigned int GetWeekdays() const { return m_iWeekdays; }

  protected:
    virtual void OnSettingChanged(const CSetting *setting);
    virtual bool AllowResettingSettings() const;
    virtual void Save();
    virtual void SetupView();
    virtual void InitializeSettings();
    
  private:
    void UpdateControlsState();

    unsigned int m_iWeekdays;
  };
} // namespace PVR
