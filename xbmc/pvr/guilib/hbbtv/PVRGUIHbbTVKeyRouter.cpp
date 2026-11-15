/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIHbbTVKeyRouter.h"

#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/InputManager.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIChannelNavigator.h"
#include "pvr/guilib/hbbtv/PVRGUIHbbTVBridge.h"
#include "pvr/hbbtv/PVRHbbTV.h"
#include "pvr/hbbtv/PVRHbbTVKeys.h"
#include "utils/log.h"

using namespace PVR;

namespace
{
// The fullscreen video or visualisation window if Kodi shows nothing taking input above the
// application, WINDOW_INVALID otherwise. A channel preview takes input without a dialog.
int MediaWindowWithInput()
{
  if (CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelNavigator().IsPreview())
    return WINDOW_INVALID;

  CGUIComponent* gui = CServiceBroker::GetGUI();
  return gui ? gui->GetWindowManager().GetMediaWindowWithInput() : WINDOW_INVALID;
}
} // unnamed namespace

CPVRGUIHbbTVKeyRouter::Target CPVRGUIHbbTVKeyRouter::OnKey(const Key& key, bool down, bool repeat)
{
  const bool pressed = down && !repeat;

  // The rest of a press that was handled as a long press is not meant for anyone.
  if (key.code == m_heldKey)
  {
    if (pressed || !down)
      m_heldKey = NO_KEY;
    if (!pressed)
      return Target::NONE;
  }

  // A press that went to Kodi stays there until its release.
  if (key.code == m_kodiKey && !pressed)
  {
    if (!down)
      m_kodiKey = NO_KEY;
    return Target::KODI;
  }

  // Kodi shows something taking input, or the key is a shortcut of Kodi rather than a key of
  // the remote.
  const int window = MediaWindowWithInput();
  if (window == WINDOW_INVALID || CPVRGUIHbbTVBridge::IsTextInputActive() || key.combination)
    return ToKodi(key, down);

  if (key.exit)
  {
    if (pressed)
      CServiceBroker::GetPVRManager().Get<PVR::CPVRHbbTV>().CloseApplication();
    return Target::NONE;
  }

  const int virtualKey = VirtualKeyFor(key.keysym.sym);
  const bool accepted = virtualKey != 0 && CPVRGUIHbbTVBridge::AcceptsKey(virtualKey);
  if (pressed)
    CLog::Log(LOGDEBUG, "HbbTV: key {} -> VK {} {}", key.code, virtualKey,
              accepted ? "for the application" : "left to Kodi");

  // Select and the keys Kodi maps a long press of: a short press is delivered on release.
  // Holding select shows the remote keys, holding another key runs Kodi's long press action.
  const bool pending = m_pendingKey && m_pendingKey->code == key.code;
  if (virtualKey == HBBTV::VK_ENTER || pending ||
      (pressed && accepted &&
       CServiceBroker::GetInputManager().HasLongpressMapping(window, key.keysym)))
  {
    if (pressed)
    {
      m_pendingKey = key;
      return Target::HOLD;
    }
    if (down || !pending)
      return Target::NONE;

    m_pendingKey.reset();
  }

  if (!accepted)
    return ToKodi(key, down);

  // Arrows, Enter and digits carry the same code in the browser and in HbbTV, so the real
  // event can go through. The other keys only exist as HbbTV codes.
  const bool sameCode = virtualKey == HBBTV::VK_ENTER ||
                        (virtualKey >= HBBTV::VK_LEFT && virtualKey <= HBBTV::VK_DOWN) ||
                        (virtualKey >= '0' && virtualKey <= '9');
  return sameCode ? Target::APPLICATION : Target::APPLICATION_VIRTUAL;
}

int CPVRGUIHbbTVKeyRouter::VirtualKeyFor(XBMCKey key)
{
  if (key >= XBMCK_0 && key <= XBMCK_9)
    return '0' + (key - XBMCK_0);

  switch (key)
  {
    case XBMCK_LEFT:
      return HBBTV::VK_LEFT;
    case XBMCK_UP:
      return HBBTV::VK_UP;
    case XBMCK_RIGHT:
      return HBBTV::VK_RIGHT;
    case XBMCK_DOWN:
      return HBBTV::VK_DOWN;
    case XBMCK_RETURN:
    case XBMCK_KP_ENTER:
      return HBBTV::VK_ENTER;
    case XBMCK_BACKSPACE:
      return HBBTV::VK_BACK;
    // The letters stand in for the colour keys on a keyboard.
    case XBMCK_RED:
    case XBMCK_r:
      return HBBTV::VK_RED;
    case XBMCK_GREEN:
    case XBMCK_g:
      return HBBTV::VK_GREEN;
    case XBMCK_YELLOW:
    case XBMCK_y:
      return HBBTV::VK_YELLOW;
    case XBMCK_BLUE:
    case XBMCK_b:
      return HBBTV::VK_BLUE;
    case XBMCK_MEDIA_PLAY_PAUSE:
      return HBBTV::VK_PLAY_PAUSE;
    case XBMCK_MEDIA_STOP:
      return HBBTV::VK_STOP;
    case XBMCK_MEDIA_REWIND:
      return HBBTV::VK_REWIND;
    case XBMCK_MEDIA_FASTFORWARD:
      return HBBTV::VK_FAST_FWD;
    default:
      return 0;
  }
}

void CPVRGUIHbbTVKeyRouter::OnKeyHeld()
{
  if (!m_pendingKey)
    return;

  const Key key = *m_pendingKey;
  m_pendingKey.reset();

  const int window = MediaWindowWithInput();
  if (window == WINDOW_INVALID)
    return;

  m_heldKey = key.code;
  if (VirtualKeyFor(key.keysym.sym) == HBBTV::VK_ENTER)
    CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTIVATE_WINDOW,
                                               WINDOW_DIALOG_PVR_REMOTE_KEYS, 0);
  else
    CServiceBroker::GetInputManager().QueueLongpress(window, key.keysym);
}

void CPVRGUIHbbTVKeyRouter::Reset()
{
  m_pendingKey.reset();
}

CPVRGUIHbbTVKeyRouter::Target CPVRGUIHbbTVKeyRouter::ToKodi(const Key& key, bool down)
{
  if (down)
    m_kodiKey = key.code;
  return Target::KODI;
}
