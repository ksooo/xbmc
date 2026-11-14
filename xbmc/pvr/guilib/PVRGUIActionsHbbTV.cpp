/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsHbbTV.h"

#include "ServiceBroker.h"
#include "input/actions/ActionIDs.h"
#include "pvr/PVRManager.h"
#include "pvr/hbbtv/PVRHbbTV.h"
#include "pvr/hbbtv/PVRHbbTVKeys.h"

using namespace PVR;

namespace
{
// HbbTV VK_* code of a colour key action, 0 for other actions.
int VirtualKeyForAction(int actionId)
{
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
  return virtualKey == HBBTV::VK_RED && HbbTVComponent().LaunchAutostartApplication();
}
