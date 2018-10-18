/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRClientCapabilities.h"

#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

namespace PVR
{

CPVRClientCapabilities::CPVRClientCapabilities(const PVR_ADDON_CAPABILITIES& addonCapabilities)
: m_addonCapabilities(new PVR_ADDON_CAPABILITIES(addonCapabilities))
{
  InitRecordingsLifetimeValues();
}

bool CPVRClientCapabilities::SupportsTV() const
{
  return m_addonCapabilities->bSupportsTV;
}

bool CPVRClientCapabilities::SupportsRadio() const
{
  return m_addonCapabilities->bSupportsRadio;
}

bool CPVRClientCapabilities::SupportsChannelGroups() const
{
  return m_addonCapabilities->bSupportsChannelGroups;
}

bool CPVRClientCapabilities::SupportsChannelScan() const
{
  return m_addonCapabilities->bSupportsChannelScan;
}

bool CPVRClientCapabilities::SupportsChannelSettings() const
{
  return m_addonCapabilities->bSupportsChannelSettings;
}

bool CPVRClientCapabilities::SupportsDescrambleInfo() const
{
  return m_addonCapabilities->bSupportsDescrambleInfo;
}

bool CPVRClientCapabilities::SupportsEPG() const
{
  return m_addonCapabilities->bSupportsEPG;
}

bool CPVRClientCapabilities::SupportsEpgTagEdl() const
{
  return m_addonCapabilities && m_addonCapabilities->bSupportsEPG && m_addonCapabilities->bSupportsEPGEdl;
}

bool CPVRClientCapabilities::SupportsAsyncEPGTransfer() const
{
  return m_addonCapabilities && m_addonCapabilities->bSupportsEPG && m_addonCapabilities->bSupportsAsyncEPGTransfer;
}

bool CPVRClientCapabilities::SupportsTimers() const
{
  return m_addonCapabilities->bSupportsTimers;
}

bool CPVRClientCapabilities::SupportsRecordings() const
{
  return m_addonCapabilities->bSupportsRecordings;
}

bool CPVRClientCapabilities::SupportsRecordingsUndelete() const
{
  return m_addonCapabilities->bSupportsRecordings && m_addonCapabilities->bSupportsRecordingsUndelete;
}

bool CPVRClientCapabilities::SupportsRecordingsPlayCount() const
{
  return m_addonCapabilities->bSupportsRecordings && m_addonCapabilities->bSupportsRecordingPlayCount;
}

bool CPVRClientCapabilities::SupportsRecordingsLastPlayedPosition() const
{
  return m_addonCapabilities->bSupportsRecordings && m_addonCapabilities->bSupportsLastPlayedPosition;
}

bool CPVRClientCapabilities::SupportsRecordingsEdl() const
{
  return m_addonCapabilities->bSupportsRecordings && m_addonCapabilities->bSupportsRecordingEdl;
}

bool CPVRClientCapabilities::SupportsRecordingsRename() const
{
  return m_addonCapabilities->bSupportsRecordings && m_addonCapabilities->bSupportsRecordingsRename;
}

bool CPVRClientCapabilities::SupportsRecordingsLifetimeChange() const
{
  return m_addonCapabilities->bSupportsRecordings && m_addonCapabilities->bSupportsRecordingsLifetimeChange;
}

bool CPVRClientCapabilities::HandlesInputStream() const
{
  return m_addonCapabilities->bHandlesInputStream;
}

bool CPVRClientCapabilities::HandlesDemuxing() const
{
  return m_addonCapabilities->bHandlesDemuxing;
}

void CPVRClientCapabilities::InitRecordingsLifetimeValues()
{
  m_recordingsLifetimeValues.clear();
  if (m_addonCapabilities->iRecordingsLifetimesSize > 0)
  {
    for (unsigned int i = 0; i < m_addonCapabilities->iRecordingsLifetimesSize; ++i)
    {
      int iValue = m_addonCapabilities->recordingsLifetimeValues[i].iValue;
      std::string strDescr(m_addonCapabilities->recordingsLifetimeValues[i].strDescription);
      if (strDescr.empty())
      {
        // No description given by addon. Create one from value.
        strDescr = StringUtils::Format("%d", iValue);
      }
      m_recordingsLifetimeValues.push_back(std::make_pair(strDescr, iValue));
    }
  }
  else if (SupportsRecordingsLifetimeChange())
  {
    // No values given by addon, but lifetime supported. Use default values 1..365
    for (int i = 1; i < 366; ++i)
    {
      m_recordingsLifetimeValues.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(17999).c_str(), i), i)); // "%s days"
    }
  }
  else
  {
    // No lifetime supported.
  }
}

void CPVRClientCapabilities::GetRecordingsLifetimeValues(std::vector<std::pair<std::string, int>> &list) const
{
  for (const auto &lifetime : m_recordingsLifetimeValues)
    list.push_back(lifetime);
}

} // namespace PVR
