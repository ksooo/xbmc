/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

#include "pvr/dialogs/GUIDialogPVRItemsViewBase.h"

class CFileItemList;

namespace PVR
{
  class CPVRChannel;

  class CGUIDialogPVRChannelGuide : public CGUIDialogPVRItemsViewBase
  {
  public:
    CGUIDialogPVRChannelGuide(void);
    ~CGUIDialogPVRChannelGuide(void) override = default;

    void Open(const std::shared_ptr<CPVRChannel> &channel);

  protected:
    void OnInitWindow() override;
    void OnDeinitWindow(int nextWindowID) override;

  private:
    std::shared_ptr<CPVRChannel> m_channel;
  };
}
