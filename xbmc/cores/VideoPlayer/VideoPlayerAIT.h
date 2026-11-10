/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDMessageQueue.h"
#include "IVideoPlayer.h"
#include "Interface/TimingConstants.h"
#include "threads/Thread.h"

#include <cstdint>
#include <memory>
#include <set>
#include <vector>

class CDVDStreamInfo;

/*!
 * Consumes a DVB Application Information Table stream. Each packet holds one complete
 * AIT section, as delivered by PVR add-ons for a PVR_CODEC_TYPE_AIT stream.
 */
class CVideoPlayerAIT : public IDVDStreamPlayer, private CThread
{
public:
  explicit CVideoPlayerAIT(CProcessInfo& processInfo);
  ~CVideoPlayerAIT() override;

  bool CheckStream(const CDVDStreamInfo& hints);
  void Flush();

  bool OpenStream(CDVDStreamInfo hints) override;
  void CloseStream(bool bWaitForBuffers) override;
  void SendMessage(std::shared_ptr<CDVDMsg> pMsg, int priority = 0) override;
  void FlushMessages() override;
  bool IsInited() const override;
  bool AcceptsData() const override;
  bool IsStalled() const override;

protected:
  void OnExit() override;
  void Process() override;

private:
  void ProcessSection(const unsigned char* data, unsigned int length);

  int m_speed = DVD_PLAYSPEED_NORMAL;
  CDVDMessageQueue m_messageQueue;
  std::set<std::vector<uint8_t>> m_seenSections;
};
