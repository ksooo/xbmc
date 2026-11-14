/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include <vector>

namespace PVR
{
/*!
 * Offers the keys the running HbbTV application asked for that input devices may lack, and
 * a button to end the application.
 */
class CGUIDialogPVRRemoteKeys : public CGUIDialog
{
public:
  CGUIDialogPVRRemoteKeys();
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;

protected:
  void OnInitWindow() override;

private:
  void MoveVertically(bool down);
  void SendKey(int virtualKey);

  std::vector<std::vector<int>> m_rows; // visible buttons per visible row, top to bottom
};
} // namespace PVR
