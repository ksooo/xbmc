/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/hbbtv/PVRHbbTV.h"

#include <atomic>
#include <string>

#include <jni.h>

namespace jni
{

//! HbbTV application engine backed by XBMCHbbTVView, WebViews above Kodi's surfaces.
class CJNIXBMCHbbTVView : public PVR::IPVRHbbTVEngine
{
public:
  static void RegisterNatives(JNIEnv* env);

  //! Install the engine in the PVR HbbTV component.
  static void Register();

  void Prepare(bool wait) override;
  void Launch(const std::string& url) override;
  void Close() override;
  bool IsRunning() const override { return m_active; }
  void LoadUrl(const std::string& url) override;
  void SetVisible(bool visible) override;
  void RunScript(const std::string& script) override;
  bool GetContentSize(float& width, float& height) const override;
  void SendKey(int virtualKey) override;

private:
  static jstring _request(JNIEnv* env, jclass clazz, jstring json);
  static jboolean _acceptsKey(JNIEnv* env, jclass clazz, jint virtualKey);
  static jboolean _isTextInputActive(JNIEnv* env, jclass clazz);
  static jstring _keyEventScript(JNIEnv* env, jclass clazz, jint virtualKey, jstring type);
  static void _onNewDocument(JNIEnv* env, jclass clazz);
  static void _onActive(JNIEnv* env, jclass clazz, jboolean active);
  static void _onExit(JNIEnv* env, jclass clazz);
  static jstring _remoteKeysScript(JNIEnv* env, jclass clazz);
  static void _forwardKey(JNIEnv* env, jclass clazz, jint keyCode, jboolean up);
  static void _log(JNIEnv* env, jclass clazz, jstring message);
  static void _logConsole(JNIEnv* env, jclass clazz, jstring message, jboolean error);
  static jstring _userAgent(JNIEnv* env, jclass clazz);

  static std::atomic<bool> m_active;
};

} // namespace jni
