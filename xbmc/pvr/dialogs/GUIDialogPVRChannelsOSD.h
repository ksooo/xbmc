/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <memory>
#include <string>

#include "threads/SystemClock.h"
#include "utils/Observer.h"

#include "pvr/PVRChannelNumberInputHandler.h"
#include "pvr/dialogs/GUIDialogPVRItemsViewBase.h"

namespace PVR
{
  class CPVRChannelGroup;

  class CGUIDialogPVRChannelsOSD : public CGUIDialogPVRItemsViewBase, public Observer, public CPVRChannelNumberInputHandler
  {
  public:
    CGUIDialogPVRChannelsOSD(void);
    ~CGUIDialogPVRChannelsOSD(void) override;
    bool OnMessage(CGUIMessage& message) override;
    bool OnAction(const CAction &action) override;

    // Observer implementation
    void Notify(const Observable &obs, const ObservableMessage msg) override;

    // CPVRChannelNumberInputHandler implementation
    void GetChannelNumbers(std::vector<std::string>& channelNumbers) override;
    void OnInputDone() override;

  protected:
    void OnInitWindow() override;
    void OnDeinitWindow(int nextWindowID) override;
    void RestoreControlStates() override;
    void SaveControlStates() override;
    void SetInvalid() override;

  private:
    void GotoChannel(int iItem);
    void Update();
    void SaveSelectedItemPath(int iGroupID);
    std::string GetLastSelectedItemPath(int iGroupID) const;

    std::shared_ptr<CPVRChannelGroup> m_group;
    std::map<int, std::string> m_groupSelectedItemPaths;
    XbmcThreads::EndTime m_refreshTimeout;
  };
}

