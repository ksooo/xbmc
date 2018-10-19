/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

struct PVR_EDL_ENTRY;

namespace PVR
{
  class CPVRClientEdlEntry
  {
  public:
    CPVRClientEdlEntry() = delete;
    virtual ~CPVRClientEdlEntry() = default;

    explicit CPVRClientEdlEntry(const PVR_EDL_ENTRY& edlEntry);

    int64_t GetStart() const;
    int64_t GetEnd() const;

    bool IsCut() const;
    bool IsMute() const;
    bool IsScene() const;
    bool IsCommBreak() const;
  
  private:
    const std::shared_ptr<PVR_EDL_ENTRY> m_edlEntry;
  };
}
