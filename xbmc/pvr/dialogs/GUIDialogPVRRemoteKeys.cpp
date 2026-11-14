/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRRemoteKeys.h"

#include "ServiceBroker.h"
#include "guilib/GUIControl.h"
#include "guilib/GUIMacros.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/hbbtv/PVRGUIHbbTVBridge.h"
#include "pvr/hbbtv/PVRHbbTV.h"
#include "pvr/hbbtv/PVRHbbTVKeys.h"

#include <algorithm>
#include <cmath>
#include <utility>

using namespace PVR;

namespace
{
constexpr int CONTROL_ROW_DIGITS = 102;
constexpr int CONTROL_ROW_VCR = 103;
constexpr int CONTROL_EXIT = 30;

// {control, HbbTV VK_* code}
constexpr std::pair<int, int> KEY_BUTTONS[] = {
    {1, HBBTV::VK_RED}, {2, HBBTV::VK_GREEN}, {3, HBBTV::VK_YELLOW}, {4, HBBTV::VK_BLUE},
    {11, '1'}, {12, '2'}, {13, '3'}, {14, '4'}, {15, '5'}, {16, '6'}, {17, '7'},
    {18, '8'}, {19, '9'}, {10, '0'},
    {20, HBBTV::VK_REWIND}, {21, HBBTV::VK_PLAY}, {22, HBBTV::VK_PAUSE},
    {23, HBBTV::VK_STOP}, {24, HBBTV::VK_FAST_FWD}};

// Row of a key button, top to bottom: colours, digits, playback.
size_t RowOfControl(int control)
{
  if (control < 10)
    return 0;
  return control < 20 ? 1 : 2;
}

// HbbTV VK_* code of the key an action stands for, 0 for other actions.
int VirtualKeyForAction(int actionId)
{
  if (actionId >= REMOTE_0 && actionId <= REMOTE_9)
    return '0' + (actionId - REMOTE_0);

  switch (actionId)
  {
    case ACTION_TELETEXT_RED:
      return HBBTV::VK_RED;
    case ACTION_TELETEXT_GREEN:
      return HBBTV::VK_GREEN;
    case ACTION_TELETEXT_YELLOW:
      return HBBTV::VK_YELLOW;
    case ACTION_TELETEXT_BLUE:
      return HBBTV::VK_BLUE;
    case ACTION_PLAYER_REWIND:
      return HBBTV::VK_REWIND;
    case ACTION_PLAYER_PLAY:
      return HBBTV::VK_PLAY;
    case ACTION_PAUSE:
      return HBBTV::VK_PAUSE;
    case ACTION_STOP:
      return HBBTV::VK_STOP;
    case ACTION_PLAYER_FORWARD:
      return HBBTV::VK_FAST_FWD;
    default:
      return 0;
  }
}

float Centre(const CGUIControl* control)
{
  const CRect& region = control->GetRenderRegion();
  return (region.x1 + region.x2) / 2;
}
} // namespace

CGUIDialogPVRRemoteKeys::CGUIDialogPVRRemoteKeys()
  : CGUIDialog(WINDOW_DIALOG_PVR_REMOTE_KEYS, "DialogPVRRemoteKeys.xml")
{
}

bool CGUIDialogPVRRemoteKeys::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    auto& hbbtv = CServiceBroker::GetPVRManager().Get<CPVRHbbTV>();
    const int control = message.GetSenderId();
    if (control == CONTROL_EXIT)
    {
      Close(true);
      hbbtv.CloseApplication();
      return true;
    }
    for (const auto& [id, virtualKey] : KEY_BUTTONS)
    {
      if (id == control)
      {
        SendKey(virtualKey);
        return true;
      }
    }
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRRemoteKeys::SendKey(int virtualKey)
{
  Close(true);
  if (const auto engine = CServiceBroker::GetPVRManager().Get<CPVRHbbTV>().GetEngine())
    engine->SendKey(virtualKey);
}

bool CGUIDialogPVRRemoteKeys::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_MOVE_UP || action.GetID() == ACTION_MOVE_DOWN)
  {
    MoveVertically(action.GetID() == ACTION_MOVE_DOWN);
    return true;
  }

  // A key the dialog offers, pressed on the input device, is chosen directly.
  const int virtualKey = VirtualKeyForAction(action.GetID());
  if (virtualKey != 0 && CPVRGUIHbbTVBridge::AcceptsKey(virtualKey))
  {
    SendKey(virtualKey);
    return true;
  }
  return CGUIDialog::OnAction(action);
}

void CGUIDialogPVRRemoteKeys::MoveVertically(bool down)
{
  const CGUIControl* focused = GetFocusedControl();
  if (!focused)
    return;

  for (size_t row = 0; row < m_rows.size(); ++row)
  {
    const auto& buttons = m_rows[row];
    if (std::find(buttons.begin(), buttons.end(), focused->GetID()) == buttons.end())
      continue;

    // Buttons differ in width, so the target row's button nearest to the focused one is
    // picked.
    const auto& target = m_rows[(row + (down ? 1 : m_rows.size() - 1)) % m_rows.size()];
    const float x = Centre(focused);
    int nearest = target.front();
    for (const int button : target)
    {
      if (std::abs(Centre(GetControl(button)) - x) < std::abs(Centre(GetControl(nearest)) - x))
        nearest = button;
    }
    SET_CONTROL_FOCUS(nearest, 0);
    return;
  }
}

void CGUIDialogPVRRemoteKeys::OnInitWindow()
{
  m_rows.assign(3, {});
  for (const auto& [control, virtualKey] : KEY_BUTTONS)
  {
    if (CPVRGUIHbbTVBridge::AcceptsKey(virtualKey))
    {
      SET_CONTROL_VISIBLE(control);
      m_rows[RowOfControl(control)].push_back(control);
    }
    else
      SET_CONTROL_HIDDEN(control);
  }
  m_rows[0].push_back(CONTROL_EXIT);

  if (m_rows[1].empty())
    SET_CONTROL_HIDDEN(CONTROL_ROW_DIGITS);
  else
    SET_CONTROL_VISIBLE(CONTROL_ROW_DIGITS);
  if (m_rows[2].empty())
    SET_CONTROL_HIDDEN(CONTROL_ROW_VCR);
  else
    SET_CONTROL_VISIBLE(CONTROL_ROW_VCR);
  std::erase_if(m_rows, [](const auto& row) { return row.empty(); });

  CGUIDialog::OnInitWindow();
  SET_CONTROL_FOCUS(m_rows[0].front(), 0);
}
