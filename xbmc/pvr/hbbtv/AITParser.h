/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/hbbtv/PVRHbbTVApplication.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace PVR
{

//! Content of one Application Information Table section (ETSI TS 102 809, 5.3.4)
struct CAITSection
{
  uint16_t applicationType{0};
  uint8_t version{0};
  bool currentNext{true};
  uint8_t sectionNumber{0};
  uint8_t lastSectionNumber{0};
  std::vector<CPVRHbbTVApplication> applications;
};

class CAITParser
{
public:
  /*!
   * \brief Parse one complete AIT section including its CRC.
   * \return false if the data is not a valid AIT section.
   */
  static bool Parse(const uint8_t* data, size_t size, CAITSection& section);
};

} // namespace PVR
