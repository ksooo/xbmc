/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/hbbtv/PVRHbbTV.h"

#include <string>

#ifdef __OBJC__
@class NSEvent;
#else
struct NSEvent;
#endif

/*!
 * HbbTV application overlay for macOS.
 *
 * Hosts a transparent WKWebView in a child window above Kodi's main window and
 * injects the ORB HbbTV polyfill. All calls are serialized onto the main thread.
 */
class CHbbTVWebView
{
public:
  //! Start the shared web content process, if not running yet.
  static void Prepare();

  //! Create the overlay and load the given HbbTV application URL.
  static void Launch(const std::string& url);

  //! Destroy the overlay, if any.
  static void Close();

  static bool IsActive();

  //! Forward a key event to the running application. Returns true if consumed.
  static bool HandleKeyEvent(NSEvent* event);

  //! Load a new URL into the running overlay (Manager.createApplication).
  static void LoadUrl(const std::string& url);

  //! Run a script in the application document.
  static void RunScript(const std::string& script);

  //! Deliver an HbbTV virtual key (press and release) to the application.
  static void SendKey(int virtualKey);

  //! Size of the Kodi window area the application plane is centred in, in pixels.
  //! Returns false if no overlay exists.
  static bool GetContentSize(float& width, float& height);

  static void SetVisible(bool visible);
};

//! Engine registration for the PVR HbbTV component.
class CHbbTVEngineOSX : public PVR::IPVRHbbTVEngine
{
public:
  static void Register();

  void Prepare(bool wait) override { CHbbTVWebView::Prepare(); }
  void Launch(const std::string& url) override { CHbbTVWebView::Launch(url); }
  void Close() override { CHbbTVWebView::Close(); }
  bool IsRunning() const override { return CHbbTVWebView::IsActive(); }
  void LoadUrl(const std::string& url) override { CHbbTVWebView::LoadUrl(url); }
  void SetVisible(bool visible) override { CHbbTVWebView::SetVisible(visible); }
  void RunScript(const std::string& script) override { CHbbTVWebView::RunScript(script); }
  bool GetContentSize(float& width, float& height) const override
  {
    return CHbbTVWebView::GetContentSize(width, height);
  }
  void SendKey(int virtualKey) override { CHbbTVWebView::SendKey(virtualKey); }
};
