/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>

namespace PVR
{

//! application_control_code values of ETSI TS 102 809, table 4
enum class HbbTVControlCode : uint8_t
{
  AUTOSTART = 0x01,
  PRESENT = 0x02,
  DESTROY = 0x03,
  KILL = 0x04,
  PREFETCH = 0x05,
  REMOTE = 0x06,
  DISABLED = 0x07,
  PLAYBACK_AUTOSTART = 0x08,
};

//! An interactive application signalled in the Application Information Table of a service.
struct CPVRHbbTVApplication
{
  uint32_t organisationId{0};
  uint16_t applicationId{0};
  uint8_t controlCode{0};
  uint8_t priority{0};
  uint8_t visibility{0};
  bool serviceBound{false};
  uint8_t usageType{0}; // application_usage_descriptor, 1 = digital teletext
  std::string name;
  std::string url; // URL base plus initial path of the HTTP transport
};

} // namespace PVR
