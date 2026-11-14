/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

#include <android/input.h>

class CAndroidKey
{
public:
  CAndroidKey() = default;
  ~CAndroidKey() = default;

  bool onKeyboardEvent(AInputEvent *event);

  static void SetHandleMediaKeys(bool enable) { m_handleMediaKeys = enable; }
  static void SetHandleSearchKeys(bool enable) { m_handleSearchKeys = enable; }
  static void XBMC_Key(uint32_t code, uint16_t key, uint16_t modifiers, uint16_t unicode, bool up);

  //! XBMCK_* key for an Android key code, XBMCK_UNKNOWN if there is none. Media keys are left
  //! to the media session during playback unless \p mediaKeys asks for them.
  static uint16_t TranslateKeyCode(int32_t keycode, bool mediaKeys = m_handleMediaKeys);

  //! XBMCKMOD_* modifiers for an Android meta state.
  static uint16_t TranslateMetaState(int32_t state);

protected:
  static bool m_handleMediaKeys;
  static bool m_handleSearchKeys;
};
