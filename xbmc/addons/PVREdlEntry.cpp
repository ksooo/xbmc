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

#include "PVREdlEntry.h"

#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "utils/log.h"

namespace PVR
{

CPVREdlEntry::CPVREdlEntry(const PVR_EDL_ENTRY& edlEntry)
: m_edlEntry(new PVR_EDL_ENTRY(edlEntry))
{
  if (edlEntry.type != PVR_EDL_TYPE_CUT &&
      edlEntry.type != PVR_EDL_TYPE_MUTE &&
      edlEntry.type != PVR_EDL_TYPE_SCENE &&
      edlEntry.type != PVR_EDL_TYPE_COMBREAK)
    CLog::Log(LOGERROR, "CPVREdlEntry - unknown PVR_EDL_TYPE value: %d", edlEntry.type);
}

int64_t CPVREdlEntry::GetStart() const
{
  return m_edlEntry->start;
}

int64_t CPVREdlEntry::GetEnd() const
{
  return m_edlEntry->end;
}

bool CPVREdlEntry::IsCut() const
{
  return m_edlEntry->type == PVR_EDL_TYPE_CUT;
}

bool CPVREdlEntry::IsMute() const
{
return m_edlEntry->type == PVR_EDL_TYPE_MUTE;
}

bool CPVREdlEntry::IsScene() const
{
  return m_edlEntry->type == PVR_EDL_TYPE_SCENE;
}

bool CPVREdlEntry::IsCommBreak() const
{
  return m_edlEntry->type == PVR_EDL_TYPE_COMBREAK;
}

} // namespace PVR
