/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayerAIT.h"

#include "DVDStreamInfo.h"
#include "Interface/DemuxPacket.h"
#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/hbbtv/AITParser.h"
#include "pvr/hbbtv/PVRHbbTV.h"
#include "utils/log.h"

using namespace std::chrono_literals;

CVideoPlayerAIT::CVideoPlayerAIT(CProcessInfo& processInfo)
  : IDVDStreamPlayer(processInfo),
    CThread("VideoPlayerAIT"),
    m_messageQueue("ait")
{
}

CVideoPlayerAIT::~CVideoPlayerAIT()
{
  StopThread();
}

bool CVideoPlayerAIT::CheckStream(const CDVDStreamInfo& hints)
{
  return hints.type == StreamType::AIT;
}

void CVideoPlayerAIT::Flush()
{
  if (m_messageQueue.IsInited())
  {
    m_messageQueue.Flush();
    m_messageQueue.Put(std::make_shared<CDVDMsg>(CDVDMsg::GENERAL_FLUSH));
  }
}

bool CVideoPlayerAIT::OpenStream(CDVDStreamInfo hints)
{
  CloseStream(true);
  m_messageQueue.Init();
  if (hints.type != StreamType::AIT)
    return false;

  Flush();
  CLog::Log(LOGINFO, "Creating AIT processor data thread");
  m_seenSections.clear();
  CServiceBroker::GetPVRManager().Get<PVR::CPVRHbbTV>().Clear();
  Create();
  return true;
}

void CVideoPlayerAIT::CloseStream(bool bWaitForBuffers)
{
  m_messageQueue.Abort();
  StopThread();
  m_messageQueue.End();
  CServiceBroker::GetPVRManager().Get<PVR::CPVRHbbTV>().Clear();
}

void CVideoPlayerAIT::SendMessage(std::shared_ptr<CDVDMsg> pMsg, int priority)
{
  if (m_messageQueue.IsInited())
    m_messageQueue.Put(pMsg, priority);
}

void CVideoPlayerAIT::FlushMessages()
{
  m_messageQueue.Flush();
}

bool CVideoPlayerAIT::IsInited() const
{
  return true;
}

bool CVideoPlayerAIT::AcceptsData() const
{
  return !m_messageQueue.IsFull();
}

bool CVideoPlayerAIT::IsStalled() const
{
  return true;
}

void CVideoPlayerAIT::OnExit()
{
  CLog::Log(LOGINFO, "AIT processor - thread end");
}

void CVideoPlayerAIT::Process()
{
  CLog::Log(LOGINFO, "AIT processor - running thread");

  while (!m_bStop)
  {
    std::shared_ptr<CDVDMsg> pMsg;
    int priority = (m_speed == DVD_PLAYSPEED_PAUSE) ? 1 : 0;
    const MsgQueueReturnCode ret = m_messageQueue.Get(pMsg, 2s, priority);

    if (ret == MSGQ_TIMEOUT)
      continue;

    if (MSGQ_IS_ERROR(ret))
    {
      if (!m_messageQueue.ReceivedAbortRequest())
        CLog::Log(LOGERROR, "MSGQ_IS_ERROR returned true ({})", ret);
      break;
    }

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      DemuxPacket* pPacket = std::static_pointer_cast<CDVDMsgDemuxerPacket>(pMsg)->GetPacket();
      if (pPacket)
        ProcessSection(pPacket->pData, pPacket->iSize);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      m_speed = std::static_pointer_cast<CDVDMsgInt>(pMsg)->m_value;
    }
  }
}

void CVideoPlayerAIT::ProcessSection(const unsigned char* data, unsigned int length)
{
  // Backends may deliver every repetition of the table.
  if (!m_seenSections.emplace(data, data + length).second)
    return;

  PVR::CAITSection section;
  if (!PVR::CAITParser::Parse(data, length, section))
  {
    // tvheadend also sends its own serialized view of the table on this stream.
    CLog::Log(LOGDEBUG, "AIT processor - skipping {} bytes that are not an AIT section", length);
    return;
  }

  CLog::Log(LOGDEBUG, "AIT processor - application type {:#06x} version {} section {}/{}: {} apps",
            section.applicationType, section.version, section.sectionNumber,
            section.lastSectionNumber, section.applications.size());
  CServiceBroker::GetPVRManager().Get<PVR::CPVRHbbTV>().OnSection(section);
}
