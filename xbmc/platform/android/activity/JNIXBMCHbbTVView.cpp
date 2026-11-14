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
#include "pvr/PVRManager.h"
#include "pvr/guilib/hbbtv/PVRGUIHbbTVBridge.h"
#include "pvr/guilib/hbbtv/PVRGUIHbbTVKeyRouter.h"
#include "pvr/hbbtv/PVRHbbTVKeys.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"
#include "windowing/android/WinSystemAndroidGLESContext.h"

#include "platform/android/activity/AndroidKey.h"
#include "platform/android/activity/XBMCApp.h"

#include <memory>

#include <android/input.h>
#include <android/keycodes.h>
#include <android/native_window_jni.h>
#include <androidjni/Context.h>
#include <androidjni/jutils-details.hpp>

using namespace jni;

namespace
{
const std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCHbbTVView";
constexpr const char* ENGINE_PRODUCT = "AppleWebKit/537.36 (KHTML, like Gecko)";

// Used on the thread of the views only.
PVR::CPVRGUIHbbTVKeyRouter s_keyRouter;

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

// The window of the application gets the media keys itself, also during playback.
XBMCKey TranslateKeyCode(jint keyCode)
{
  return static_cast<XBMCKey>(CAndroidKey::TranslateKeyCode(keyCode, true));
}

XBMC_keysym KeysymFor(jint keyCode)
{
  XBMC_keysym keysym = {};
  keysym.scancode = keyCode;
  keysym.sym = TranslateKeyCode(keyCode);
  return keysym;
}

// The scripts go over as bytes: a Java string made by JNI only takes modified UTF-8.
jbyteArray ToByteArray(JNIEnv* env, const std::string& text)
{
  if (text.empty())
    return nullptr;

  const jsize size = static_cast<jsize>(text.size());
  jbyteArray bytes = env->NewByteArray(size);
  if (bytes)
    env->SetByteArrayRegion(bytes, 0, size, reinterpret_cast<const jbyte*>(text.data()));
  return bytes;
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
        {"_polyfillScript", "(Ljava/lang/String;)[B", (void*)&_polyfillScript},
        {"_playerPage", "(Ljava/lang/String;)[B", (void*)&_playerPage},
        {"_fetchDocument", "(Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/Object;",
         (void*)&_fetchDocument},
        {"_keyEventScript", "(ILjava/lang/String;)Ljava/lang/String;", (void*)&_keyEventScript},
        {"_onNewDocument", "()V", (void*)&_onNewDocument},
        {"_onActive", "(Z)V", (void*)&_onActive},
        {"_virtualKeyFor", "(I)I", (void*)&_virtualKeyFor},
        {"_routeKey", "(IIZZ)I", (void*)&_routeKey},
        {"_onKeyHeld", "()V", (void*)&_onKeyHeld},
        {"_forwardKey", "(IIZ)V", (void*)&_forwardKey},
        {"_setTopLayerSurface", "(Landroid/view/Surface;)V", (void*)&_setTopLayerSurface},
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
  CallView("prepare", "(Z)V", static_cast<jboolean>(wait));
}

void CJNIXBMCHbbTVView::Launch(const std::string& url)
{
  CallView("launch", "(Ljava/lang/String;)V", jcast<jhstring>(url));
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

jstring CJNIXBMCHbbTVView::_request(JNIEnv* env, jclass clazz, jstring json)
{
  const std::string response =
      PVR::CPVRGUIHbbTVBridge::HandleRequest(jcast<std::string>(jhstring::fromJNI(json)));
  return env->NewStringUTF(response.c_str());
}

jbyteArray CJNIXBMCHbbTVView::_polyfillScript(JNIEnv* env, jclass clazz, jstring url)
{
  return ToByteArray(
      env, PVR::CPVRGUIHbbTVBridge::PolyfillScript(jcast<std::string>(jhstring::fromJNI(url))));
}

jbyteArray CJNIXBMCHbbTVView::_playerPage(JNIEnv* env, jclass clazz, jstring url)
{
  return ToByteArray(
      env, PVR::CPVRGUIHbbTVBridge::PlayerPage(jcast<std::string>(jhstring::fromJNI(url))));
}

jobjectArray CJNIXBMCHbbTVView::_fetchDocument(JNIEnv* env,
                                               jclass clazz,
                                               jstring url,
                                               jstring cookies)
{
  PVR::CPVRGUIHbbTVBridge::Document document;
  const bool fetched = PVR::CPVRGUIHbbTVBridge::FetchDocument(
      jcast<std::string>(jhstring::fromJNI(url)), ENGINE_PRODUCT,
      jcast<std::string>(jhstring::fromJNI(cookies)), document);
  if (!fetched || document.body.empty())
    return nullptr;

  const jsize cookieCount = static_cast<jsize>(document.cookies.size());
  jobjectArray cookieArray =
      env->NewObjectArray(cookieCount, env->FindClass("java/lang/String"), nullptr);
  for (jsize i = 0; i < cookieCount; ++i)
  {
    jstring cookie = env->NewStringUTF(document.cookies[i].c_str());
    env->SetObjectArrayElement(cookieArray, i, cookie);
    env->DeleteLocalRef(cookie);
  }

  // {URL, MIME type, charset, body, cookies}
  jobjectArray result = env->NewObjectArray(5, env->FindClass("java/lang/Object"), nullptr);
  env->SetObjectArrayElement(result, 0, env->NewStringUTF(document.url.c_str()));
  env->SetObjectArrayElement(result, 1, env->NewStringUTF(document.mimeType.c_str()));
  env->SetObjectArrayElement(result, 2, env->NewStringUTF(document.charset.c_str()));
  env->SetObjectArrayElement(result, 3, ToByteArray(env, document.body));
  env->SetObjectArrayElement(result, 4, cookieArray);
  return result;
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
  {
    s_keyRouter.Reset();
    PVR::CPVRGUIHbbTVBridge::ResetBroadcastPresentation();
  }
}

jint CJNIXBMCHbbTVView::_virtualKeyFor(JNIEnv* env, jclass clazz, jint keyCode)
{
  return PVR::CPVRGUIHbbTVKeyRouter::VirtualKeyFor(TranslateKeyCode(keyCode));
}

jint CJNIXBMCHbbTVView::_routeKey(
    JNIEnv* env, jclass clazz, jint keyCode, jint metaState, jboolean down, jboolean repeat)
{
  PVR::CPVRGUIHbbTVKeyRouter::Key key;
  key.code = keyCode;
  key.keysym = KeysymFor(keyCode);
  key.combination = (metaState & (AMETA_CTRL_ON | AMETA_ALT_ON | AMETA_SHIFT_ON)) != 0;
  // TV remotes have no separate exit key: Back ends the application unless it asked for
  // navigation keys, in which case it is VK_BACK for the application.
  key.exit =
      keyCode == AKEYCODE_ESCAPE ||
      (keyCode == AKEYCODE_BACK && !PVR::CPVRGUIHbbTVBridge::AcceptsKey(PVR::HBBTV::VK_BACK));
  return static_cast<jint>(s_keyRouter.OnKey(key, down, repeat));
}

void CJNIXBMCHbbTVView::_onKeyHeld(JNIEnv* env, jclass clazz)
{
  s_keyRouter.OnKeyHeld();
}

void CJNIXBMCHbbTVView::_forwardKey(
    JNIEnv* env, jclass clazz, jint keyCode, jint metaState, jboolean up)
{
  CAndroidKey::XBMC_Key(keyCode, TranslateKeyCode(keyCode),
                        CAndroidKey::TranslateMetaState(metaState), 0, up);
}

void CJNIXBMCHbbTVView::_setTopLayerSurface(JNIEnv* env, jclass clazz, jobject surface)
{
  auto* winSystem = dynamic_cast<CWinSystemAndroidGLESContext*>(CServiceBroker::GetWinSystem());
  if (winSystem)
    winSystem->SetTopLayerWindow(surface ? ANativeWindow_fromSurface(env, surface) : nullptr);
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
  return env->NewStringUTF(PVR::CPVRGUIHbbTVBridge::UserAgent(ENGINE_PRODUCT).c_str());
}
