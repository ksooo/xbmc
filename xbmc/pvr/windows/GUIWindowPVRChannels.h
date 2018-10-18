/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

#include "pvr/PVRChannelNumberInputHandler.h"
#include "pvr/windows/GUIWindowPVRBase.h"

class CFileItem;

namespace PVR
{
  class CGUIWindowPVRChannelsBase : public CGUIWindowPVRBase, public CPVRChannelNumberInputHandler
  {
  public:
    CGUIWindowPVRChannelsBase(bool bRadio, int id, const std::string &xmlFile);
    ~CGUIWindowPVRChannelsBase() override;

    bool OnMessage(CGUIMessage& message) override;
    void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
    bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
    void UpdateButtons(void) override;
    bool OnAction(const CAction &action) override;

    // CPVRChannelNumberInputHandler implementation
    void GetChannelNumbers(std::vector<std::string>& channelNumbers) override;
    void OnInputDone() override;

  private:
    bool OnContextButtonManage(const std::shared_ptr<CFileItem> &item, CONTEXT_BUTTON button);

    void ShowChannelManager();
    void ShowGroupManager();
    void UpdateEpg(const std::shared_ptr<CFileItem> &item);

  protected:
    bool m_bShowHiddenChannels;
  };

  class CGUIWindowPVRTVChannels : public CGUIWindowPVRChannelsBase
  {
  public:
    CGUIWindowPVRTVChannels();

  protected:
    std::string GetDirectoryPath() override;
  };

  class CGUIWindowPVRRadioChannels : public CGUIWindowPVRChannelsBase
  {
  public:
    CGUIWindowPVRRadioChannels();

  protected:
    std::string GetDirectoryPath() override;
  };
}
