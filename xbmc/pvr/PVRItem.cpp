/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRItem.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"

namespace PVR
{
  std::shared_ptr<CPVREpgInfoTag> CPVRItem::GetEpgInfoTag() const
  {
    if (m_item->IsEPG())
    {
      return m_item->GetEPGInfoTag();
    }
    else if (m_item->IsPVRChannel())
    {
      return m_item->GetPVRChannelInfoTag()->GetEPGNow();
    }
    else if (m_item->IsPVRTimer())
    {
      return m_item->GetPVRTimerInfoTag()->GetEpgInfoTag();
    }
    else
    {
      CLog::LogF(LOGERROR, "Unsupported item type!");
    }
    return std::shared_ptr<CPVREpgInfoTag>();
  }

  std::shared_ptr<CPVREpgInfoTag> CPVRItem::GetNextEpgInfoTag() const
  {
    if (m_item->IsEPG())
    {
      const std::shared_ptr<CPVRChannel> channel = m_item->GetEPGInfoTag()->Channel();
      if (channel)
        return channel->GetEPGNext();
    }
    else if (m_item->IsPVRChannel())
    {
      return m_item->GetPVRChannelInfoTag()->GetEPGNext();
    }
    else if (m_item->IsPVRTimer())
    {
      const std::shared_ptr<CPVRChannel> channel =m_item->GetPVRTimerInfoTag()->Channel();
      if (channel)
        return channel->GetEPGNext();
    }
    else
    {
      CLog::LogF(LOGERROR, "Unsupported item type!");
    }
    return std::shared_ptr<CPVREpgInfoTag>();
  }

  std::shared_ptr<CPVRChannel> CPVRItem::GetChannel() const
  {
    if (m_item->IsPVRChannel())
    {
      return m_item->GetPVRChannelInfoTag();
    }
    else if (m_item->IsEPG())
    {
      return m_item->GetEPGInfoTag()->Channel();
    }
    else if (m_item->IsPVRTimer())
    {
      return m_item->GetPVRTimerInfoTag()->Channel();
    }
    else
    {
      CLog::LogF(LOGERROR, "Unsupported item type!");
    }
    return std::shared_ptr<CPVRChannel>();
  }

  std::shared_ptr<CPVRTimerInfoTag> CPVRItem::GetTimerInfoTag() const
  {
    if (m_item->IsPVRTimer())
    {
      return m_item->GetPVRTimerInfoTag();
    }
    else if (m_item->IsEPG())
    {
      return m_item->GetEPGInfoTag()->Timer();
    }
    else if (m_item->IsPVRChannel())
    {
      std::shared_ptr<CPVRTimerInfoTag> timer;
      const std::shared_ptr<CPVREpgInfoTag> epgTag(m_item->GetPVRChannelInfoTag()->GetEPGNow());
      if (epgTag)
        timer = epgTag->Timer(); // cheap method, but not reliable as timers get set at epg tags asynchronously

      if (timer)
        return timer;

      return CServiceBroker::GetPVRManager().Timers()->GetActiveTimerForChannel(m_item->GetPVRChannelInfoTag()); // more expensive, but reliable and works even for channels with no epg data
    }
    else
    {
      CLog::LogF(LOGERROR, "Unsupported item type!");
    }
    return std::shared_ptr<CPVRTimerInfoTag>();
  }

  std::shared_ptr<CPVRRecording> CPVRItem::GetRecording() const
  {
    if (m_item->IsPVRRecording())
    {
      return m_item->GetPVRRecordingInfoTag();
    }
    else if (m_item->IsEPG())
    {
      return m_item->GetEPGInfoTag()->Recording();
    }
    else
    {
      CLog::LogF(LOGERROR, "Unsupported item type!");
    }
    return std::shared_ptr<CPVRRecording>();
  }

  bool CPVRItem::IsRadio() const
  {
    if (m_item->IsPVRChannel())
    {
      return m_item->GetPVRChannelInfoTag()->IsRadio();
    }
    else if (m_item->IsEPG())
    {
      const std::shared_ptr<CPVRChannel> channel(m_item->GetEPGInfoTag()->Channel());
      return (channel && channel->IsRadio());
    }
    else if (m_item->IsPVRRecording())
    {
      return m_item->GetPVRRecordingInfoTag()->IsRadio();
    }
    else
    {
      CLog::LogF(LOGERROR, "Unsupported item type!");
    }
    return false;
  }

} // namespace PVR
