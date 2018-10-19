/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRClientEdlEntry.h"

#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "utils/log.h"

namespace PVR
{

CPVRClientEdlEntry::CPVRClientEdlEntry(const PVR_EDL_ENTRY& edlEntry)
: m_edlEntry(new PVR_EDL_ENTRY(edlEntry))
{
  if (edlEntry.type != PVR_EDL_TYPE_CUT &&
      edlEntry.type != PVR_EDL_TYPE_MUTE &&
      edlEntry.type != PVR_EDL_TYPE_SCENE &&
      edlEntry.type != PVR_EDL_TYPE_COMBREAK)
    CLog::Log(LOGERROR, "CPVRClientEdlEntry - unknown PVR_EDL_TYPE value: %d", edlEntry.type);
}

int64_t CPVRClientEdlEntry::GetStart() const
{
  return m_edlEntry->start;
}

int64_t CPVRClientEdlEntry::GetEnd() const
{
  return m_edlEntry->end;
}

bool CPVRClientEdlEntry::IsCut() const
{
  return m_edlEntry->type == PVR_EDL_TYPE_CUT;
}

bool CPVRClientEdlEntry::IsMute() const
{
return m_edlEntry->type == PVR_EDL_TYPE_MUTE;
}

bool CPVRClientEdlEntry::IsScene() const
{
  return m_edlEntry->type == PVR_EDL_TYPE_SCENE;
}

bool CPVRClientEdlEntry::IsCommBreak() const
{
  return m_edlEntry->type == PVR_EDL_TYPE_COMBREAK;
}

} // namespace PVR
