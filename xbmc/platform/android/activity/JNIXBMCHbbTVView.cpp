/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCHbbTVView.h"

#include "CompileInfo.h"
#include "ServiceBroker.h"
#include "filesystem/SpecialProtocol.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/hbbtv/PVRGUIHbbTVBridge.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include "platform/android/activity/AndroidKey.h"
#include "platform/android/activity/XBMCApp.h"

#include <memory>

#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

namespace
{
const std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCHbbTVView";
constexpr const char* POLYFILL_DIR = "special://xbmc/system/hbbtv/";

jhclass ViewClass()
{
  std::string dotted = s_className;
  StringUtils::Replace(dotted, '/', '.');
  return CJNIContext::getClassLoader().loadClass(dotted);
}

template<typename... Args>
void CallView(const char* name, const char* signature, Args&&... args)
{
  call_static_method<void>(xbmc_jnienv(), ViewClass(), name, signature,
                           std::forward<Args>(args)...);
}
} // unnamed namespace

std::atomic<bool> CJNIXBMCHbbTVView::m_active{false};

void CJNIXBMCHbbTVView::RegisterNatives(JNIEnv* env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if (cClass)
  {
    JNINativeMethod methods[] = {
        {"_request", "(Ljava/lang/String;)Ljava/lang/String;", (void*)&_request},
        {"_acceptsKey", "(I)Z", (void*)&_acceptsKey},
        {"_isTextInputActive", "()Z", (void*)&_isTextInputActive},
        {"_keyEventScript", "(ILjava/lang/String;)Ljava/lang/String;", (void*)&_keyEventScript},
        {"_onNewDocument", "()V", (void*)&_onNewDocument},
        {"_onActive", "(Z)V", (void*)&_onActive},
        {"_onExit", "()V", (void*)&_onExit},
        {"_remoteKeysScript", "()Ljava/lang/String;", (void*)&_remoteKeysScript},
        {"_forwardKey", "(IZ)V", (void*)&_forwardKey},
        {"_log", "(Ljava/lang/String;)V", (void*)&_log},
        {"_logConsole", "(Ljava/lang/String;Z)V", (void*)&_logConsole},
        {"_userAgent", "()Ljava/lang/String;", (void*)&_userAgent},
    };
    env->RegisterNatives(cClass, methods, sizeof(methods) / sizeof(methods[0]));
  }
}

void CJNIXBMCHbbTVView::Register()
{
  CServiceBroker::GetPVRManager().Get<PVR::CPVRHbbTV>().SetEngine(
      std::make_shared<CJNIXBMCHbbTVView>());
}

void CJNIXBMCHbbTVView::Prepare(bool wait)
{
  CallView("prepare", "(Ljava/lang/String;Z)V",
           jcast<jhstring>(CSpecialProtocol::TranslatePath(POLYFILL_DIR)),
           static_cast<jboolean>(wait));
}

void CJNIXBMCHbbTVView::Launch(const std::string& url)
{
  CallView("launch", "(Ljava/lang/String;Ljava/lang/String;)V", jcast<jhstring>(url),
           jcast<jhstring>(CSpecialProtocol::TranslatePath(POLYFILL_DIR)));
}

void CJNIXBMCHbbTVView::Close()
{
  CallView("close", "()V");
}

void CJNIXBMCHbbTVView::LoadUrl(const std::string& url)
{
  CallView("load", "(Ljava/lang/String;)V", jcast<jhstring>(url));
}

void CJNIXBMCHbbTVView::SetVisible(bool visible)
{
  CallView("show", "(Z)V", static_cast<jboolean>(visible));
}

void CJNIXBMCHbbTVView::RunScript(const std::string& script)
{
  CallView("evaluate", "(Ljava/lang/String;)V", jcast<jhstring>(script));
}

bool CJNIXBMCHbbTVView::GetContentSize(float& width, float& height) const
{
  // The application plane is centred on the whole screen, which is Kodi's GUI area.
  const auto& gfx = CServiceBroker::GetWinSystem()->GetGfxContext();
  width = static_cast<float>(gfx.GetWidth());
  height = static_cast<float>(gfx.GetHeight());
  return m_active && width > 0 && height > 0;
}

void CJNIXBMCHbbTVView::SendKey(int virtualKey)
{
  CallView("sendKey", "(I)V", static_cast<jint>(virtualKey));
}

void CJNIXBMCHbbTVView::SetOpacity(float opacity, int fadeMs)
{
  CallView("setOpacity", "(DI)V", static_cast<jdouble>(opacity), static_cast<jint>(fadeMs));
}

jstring CJNIXBMCHbbTVView::_request(JNIEnv* env, jclass clazz, jstring json)
{
  const std::string response =
      PVR::CPVRGUIHbbTVBridge::HandleRequest(jcast<std::string>(jhstring::fromJNI(json)));
  return env->NewStringUTF(response.c_str());
}

jboolean CJNIXBMCHbbTVView::_acceptsKey(JNIEnv* env, jclass clazz, jint virtualKey)
{
  return PVR::CPVRGUIHbbTVBridge::AcceptsKey(virtualKey);
}

jboolean CJNIXBMCHbbTVView::_isTextInputActive(JNIEnv* env, jclass clazz)
{
  return PVR::CPVRGUIHbbTVBridge::IsTextInputActive();
}

jstring CJNIXBMCHbbTVView::_keyEventScript(JNIEnv* env,
                                           jclass clazz,
                                           jint virtualKey,
                                           jstring type)
{
  const std::string script = PVR::CPVRGUIHbbTVBridge::KeyEventScript(
      virtualKey, jcast<std::string>(jhstring::fromJNI(type)));
  return env->NewStringUTF(script.c_str());
}

void CJNIXBMCHbbTVView::_onNewDocument(JNIEnv* env, jclass clazz)
{
  PVR::CPVRGUIHbbTVBridge::OnNewDocument();
}

void CJNIXBMCHbbTVView::_onActive(JNIEnv* env, jclass clazz, jboolean active)
{
  m_active = active;
  // Media playing inside the application's web view takes the audio focus. The application
  // is part of Kodi's own playback and controls the broadcast itself.
  CXBMCApp::Get().SetIgnoreAudioFocusLoss(active);
  if (!active)
    PVR::CPVRGUIHbbTVBridge::ResetBroadcastPresentation();
}

void CJNIXBMCHbbTVView::_onExit(JNIEnv* env, jclass clazz)
{
  CServiceBroker::GetPVRManager().Get<PVR::CPVRHbbTV>().CloseApplication();
}

jstring CJNIXBMCHbbTVView::_remoteKeysScript(JNIEnv* env, jclass clazz)
{
  return env->NewStringUTF(PVR::CPVRGUIHbbTVBridge::RemoteKeysScript().c_str());
}

void CJNIXBMCHbbTVView::_forwardKey(JNIEnv* env, jclass clazz, jint keyCode, jboolean up)
{
  CAndroidKey::XBMC_Key(keyCode, CAndroidKey::TranslateKeyCode(keyCode), 0, 0, up);
}

void CJNIXBMCHbbTVView::_log(JNIEnv* env, jclass clazz, jstring message)
{
  CLog::Log(LOGINFO, "HbbTV: {}", jcast<std::string>(jhstring::fromJNI(message)));
}

void CJNIXBMCHbbTVView::_logConsole(JNIEnv* env, jclass clazz, jstring message, jboolean error)
{
  PVR::CPVRGUIHbbTVBridge::LogConsole(jcast<std::string>(jhstring::fromJNI(message)), error);
}

jstring CJNIXBMCHbbTVView::_userAgent(JNIEnv* env, jclass clazz)
{
  return env->NewStringUTF(
      PVR::CPVRGUIHbbTVBridge::UserAgent("AppleWebKit/537.36 (KHTML, like Gecko)").c_str());
}
