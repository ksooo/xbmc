/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

#include "guilib/GUIDialog.h"

class CFileItem;

namespace PVR
{
  class CGUIDialogPVRRecordingInfo : public CGUIDialog
  {
  public:
    CGUIDialogPVRRecordingInfo(void);
    ~CGUIDialogPVRRecordingInfo(void) override = default;
    bool OnMessage(CGUIMessage& message) override;
    bool OnInfo(int actionID) override;
    bool HasListItems() const override { return true; }
    std::shared_ptr<CFileItem> GetCurrentListItem(int offset = 0) override;

    void SetRecording(const CFileItem *item);

    static void ShowFor(const std::shared_ptr<CFileItem>& item);

  private:
    bool OnClickButtonOK(CGUIMessage &message);
    bool OnClickButtonPlay(CGUIMessage &message);

    std::shared_ptr<CFileItem> m_recordItem;
  };
}
