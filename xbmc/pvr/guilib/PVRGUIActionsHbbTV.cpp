/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsHbbTV.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/ActionIDs.h"
#include "pvr/PVRManager.h"
#include "pvr/hbbtv/PVRHbbTV.h"

using namespace PVR;

namespace
{
// HbbTV virtual key codes of the colour keys
constexpr int VK_RED = 403;
constexpr int VK_GREEN = 404;
constexpr int VK_YELLOW = 405;
constexpr int VK_BLUE = 406;

// Kodi draws its GUI beneath the application, so a notification shows through the dimmed
// application only.
constexpr float DIMMED_OPACITY = 0.3f;
constexpr int DIM_FADE_MS = 200;

// HbbTV VK_* code of a colour key action, 0 for other actions.
int VirtualKeyForAction(int actionId)
{
  switch (actionId)
  {
    case ACTION_TELETEXT_RED:
      return VK_RED;
    case ACTION_TELETEXT_GREEN:
      return VK_GREEN;
    case ACTION_TELETEXT_YELLOW:
      return VK_YELLOW;
    case ACTION_TELETEXT_BLUE:
      return VK_BLUE;
    default:
      return 0;
  }
}

CPVRHbbTV& HbbTVComponent()
{
  return CServiceBroker::GetPVRManager().Get<CPVRHbbTV>();
}
} // namespace

bool CPVRGUIActionsHbbTV::OnColourKey(int actionId) const
{
  const int virtualKey = VirtualKeyForAction(actionId);
  if (virtualKey == 0)
    return false;

  const auto engine = HbbTVComponent().GetEngine();
  if (engine && engine->IsRunning())
  {
    engine->SendKey(virtualKey);
    return true;
  }
  return virtualKey == VK_RED && HbbTVComponent().LaunchAutostartApplication();
}

void CPVRGUIActionsHbbTV::UpdateApplicationOpacity()
{
  const auto engine = HbbTVComponent().GetEngine();
  if (!engine)
    return;

  const CGUIWindow* toast =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_DIALOG_KAI_TOAST);
  const bool dimmed = engine->IsRunning() && toast && toast->IsDialogRunning();
  if (dimmed != m_dimmed)
  {
    m_dimmed = dimmed;
    engine->SetOpacity(dimmed ? DIMMED_OPACITY : 1.0f, DIM_FADE_MS);
  }
}
