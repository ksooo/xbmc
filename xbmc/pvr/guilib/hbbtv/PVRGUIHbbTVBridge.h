/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace PVR
{

/*!
 * Native side of the ORB polyfill bridge, shared by all application engines.
 *
 * Takes the JSON request the polyfill sends through hbbtv.native.request()
 * and returns the JSON response. Broadcast related calls are mapped onto
 * Kodi's PVR manager, everything else gets a static default answer.
 */
class CPVRGUIHbbTVBridge
{
public:
  static std::string HandleRequest(const std::string& json);

  //! Whether the running application asked for the key set the given HbbTV VK_* code belongs to.
  static bool AcceptsKey(int virtualKey);

  //! Kodi's keyboard dialog is collecting text for the application; keys stay with Kodi.
  static bool IsTextInputActive();

  //! A new document started in the view: fresh key set, broadcast back to full screen.
  static void OnNewDocument();

  //! Broadcast back to full screen and audible, e.g. when the application goes away.
  static void ResetBroadcastPresentation();

  //! Script that dispatches a key event with the given HbbTV VK_* code to the focused element.
  static std::string KeyEventScript(int virtualKey, const std::string& type);

  //! Script that shows the remote keys of Kodi's overlay page (kodioverlay.html) with the keys of
  //! the application's key set and a button that ends the application.
  static std::string RemoteKeysScript();

  //! User-Agent for the application's requests, ending with the given browser engine product.
  static std::string UserAgent(const std::string& engine);

  //! Logs a console message of the application; errors as warning, the rest as PVR debug log.
  static void LogConsole(const std::string& message, bool error);
};

} // namespace PVR
