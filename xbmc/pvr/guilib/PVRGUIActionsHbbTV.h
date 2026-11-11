/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/IPVRComponent.h"

namespace PVR
{
class CPVRGUIActionsHbbTV : public IPVRComponent
{
public:
  CPVRGUIActionsHbbTV() = default;
  ~CPVRGUIActionsHbbTV() override = default;

  /*!
   * Handle a colour key action (ACTION_TELETEXT_RED .. ACTION_TELETEXT_BLUE). A running
   * application receives the key; otherwise red starts the autostart application.
   * \return true if the action was consumed.
   */
  bool OnColourKey(int actionId) const;

  //! Dim the running application while a notification is shown. Call once per frame.
  void UpdateApplicationOpacity();

private:
  CPVRGUIActionsHbbTV(const CPVRGUIActionsHbbTV&) = delete;
  CPVRGUIActionsHbbTV const& operator=(CPVRGUIActionsHbbTV const&) = delete;

  bool m_dimmed{false};
};

namespace GUI
{
// pretty scope and name
using HbbTV = CPVRGUIActionsHbbTV;
} // namespace GUI

} // namespace PVR
