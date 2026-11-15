/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keyboard/XBMC_keyboard.h"

#include <optional>

namespace PVR
{

/*!
 * Decides who gets a key pressed while an application is running, shared by all application
 * engines.
 *
 * Like on a TV set, the application only gets the keys it asked for. Kodi gets all keys while
 * it shows something taking input, key combinations, and the long press of a key it maps one
 * for. Not thread-safe: to be used on the thread the engine gets its keys on.
 */
class CPVRGUIHbbTVKeyRouter
{
public:
  struct Key
  {
    int code{0}; //!< the platform's code of the key, tells the keys apart
    XBMC_keysym keysym{}; //!< the key as Kodi knows it
    bool combination{false}; //!< pressed together with a modifier key
    bool exit{false}; //!< the key ends the application
  };

  enum class Target
  {
    APPLICATION, //!< the page gets the event itself
    APPLICATION_VIRTUAL, //!< the page gets the HbbTV code by CPVRGUIHbbTVBridge::KeyEventScript()
    KODI,
    NONE, //!< drop the event
    HOLD, //!< keep the event, call OnKeyHeld() if the key is still down after the hold time
  };

  /*!
   * \brief Get the target of a key event.
   * The release of a kept key that names the application or Kodi as target is preceded by
   * the kept event, which has the same target.
   */
  Target OnKey(const Key& key, bool down, bool repeat);

  //! HbbTV VK_* code of a key, 0 if it has none.
  static int VirtualKeyFor(XBMCKey key);

  //! The key kept for Target::HOLD is still down after KODI::KEYBOARD::KEY_HOLD_TRESHOLD.
  void OnKeyHeld();

  //! Forget the kept key, as the application ended.
  void Reset();

private:
  Target ToKodi(const Key& key, bool down);

  static constexpr int NO_KEY = -1;

  std::optional<Key> m_pendingKey; //!< key down kept until its release or the hold time
  int m_heldKey{NO_KEY}; //!< key still down from a press that was handled as a long press
  int m_kodiKey{NO_KEY}; //!< key whose press went to Kodi
};

} // namespace PVR
