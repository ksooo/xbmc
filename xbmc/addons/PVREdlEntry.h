#pragma once
/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <memory>

struct PVR_EDL_ENTRY;

namespace PVR
{
  class CPVREdlEntry
  {
  public:
    CPVREdlEntry() = delete;
    virtual ~CPVREdlEntry() = default;

    explicit CPVREdlEntry(const PVR_EDL_ENTRY& edlEntry);

    int64_t GetStart() const;
    int64_t GetEnd() const;

    bool IsCut() const;
    bool IsMute() const;
    bool IsScene() const;
    bool IsCommBreak() const;
  
  private:
    std::shared_ptr<PVR_EDL_ENTRY> m_edlEntry;
  };
}
