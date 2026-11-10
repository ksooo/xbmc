/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

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
  //! Accept header for requests of application documents.
  static constexpr const char* ACCEPT_HEADER =
      "application/vnd.hbbtv.xhtml+xml,application/xhtml+xml,text/html;q=0.9,*/*;q=0.8";

  struct Document
  {
    std::string url; //!< where the document came from, after redirects
    std::string mimeType; //!< XHTML for the HbbTV type, which web views refuse
    std::string charset;
    std::string body;
    std::vector<std::string> cookies; //!< values of the Set-Cookie headers
  };

  static std::string HandleRequest(const std::string& json);

  /*!
   * \brief Fetch an application document with Kodi's HTTP client.
   * \param url the document to fetch
   * \param engine the browser engine product for the User-Agent, see UserAgent()
   * \param cookies the value of the Cookie header, empty for none
   * \param document the fetched document
   * \return true if the document was fetched
   */
  static bool FetchDocument(const std::string& url,
                            const std::string& engine,
                            const std::string& cookies,
                            Document& document);

  //! The polyfill for the document at the given URL, to run before the scripts of the
  //! document. Empty if the polyfill is not available.
  static std::string PolyfillScript(const std::string& url);

  //! The polyfill's internal player page (orb://player) for the given URL. The polyfill loads
  //! broadband media in an iframe with this page, which hosts the actual video element.
  //! Empty if the polyfill is not available.
  static std::string PlayerPage(const std::string& url);

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

  //! User-Agent for the application's requests, ending with the given browser engine product.
  static std::string UserAgent(const std::string& engine);

  //! Logs a console message of the application; errors as warning, the rest as PVR debug log.
  static void LogConsole(const std::string& message, bool error);
};

} // namespace PVR
