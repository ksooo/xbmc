/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/IPVRComponent.h"
#include "pvr/hbbtv/PVRHbbTVApplication.h"
#include "threads/CriticalSection.h"
#include "threads/Timer.h"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace PVR
{
struct CAITSection;

/*!
 * Platform side of HbbTV: runs one interactive application at a time.
 */
class IPVRHbbTVEngine
{
public:
  virtual ~IPVRHbbTVEngine() = default;

  /*!
   * Start the browser runtime ahead of time, so the first application opens without a stall.
   * @param wait Return only once the runtime is ready. Engines whose start does not stall
   * Kodi's video output may ignore it.
   */
  virtual void Prepare(bool wait) = 0;

  virtual void Launch(const std::string& url) = 0;
  virtual void Close() = 0;
  virtual bool IsRunning() const = 0;

  //! Load another URL into the running application view (application handover).
  virtual void LoadUrl(const std::string& url) = 0;

  virtual void SetVisible(bool visible) = 0;

  //! Run a script in the application document.
  virtual void RunScript(const std::string& script) = 0;

  //! Size of the window area the application plane is centred in, in pixels.
  virtual bool GetContentSize(float& width, float& height) const = 0;

  //! Deliver an HbbTV virtual key code (VK_*) to the running application.
  virtual void SendKey(int virtualKey) = 0;
};

/*!
 * Interactive applications signalled for the channel currently being played. Fed by the
 * AIT stream of the playing channel, empty whenever no such stream is open.
 */
class CPVRHbbTV : public IPVRComponent
{
public:
  //! Take over the applications of one AIT section. A new table version replaces all sections.
  void OnSection(const CAITSection& section);

  //! Forget the applications and terminate a running one. Called when the AIT stream closes.
  void Clear();

  //! Playback of a new item started, its picture is not settled yet.
  void OnPlaybackStarted();

  //! Audio and video of the playing item are in sync.
  void OnAVStarted();

  //! Start the platform's browser engine ahead of the first application, if HbbTV is enabled.
  void PrepareEngine(bool wait);

  //! Install the platform's application engine. Without one, applications cannot be started.
  void SetEngine(std::shared_ptr<IPVRHbbTVEngine> engine);
  std::shared_ptr<IPVRHbbTVEngine> GetEngine() const;

  bool HasApplications() const;
  std::vector<CPVRHbbTVApplication> GetApplications() const;

  //! The autostart application with the highest priority, if any.
  std::optional<CPVRHbbTVApplication> GetAutostartApplication() const;

  //! Start the application at the given URL. Returns false without an engine.
  bool LaunchApplication(const std::string& url);

  //! Start the autostart application of the playing channel, if there is one.
  bool LaunchAutostartApplication();

  void CloseApplication();
  bool IsApplicationRunning() const;

private:
  void ScheduleAutostart();
  void OnAutostartTimer();

  mutable CCriticalSection m_critSection;
  std::optional<uint8_t> m_version;
  bool m_autostartDone{false}; // once per channel, so a closed application stays closed
  bool m_avStarted{false};
  CTimer m_autostartTimer{[this]() { OnAutostartTimer(); }};
  std::map<uint8_t, std::vector<CPVRHbbTVApplication>> m_sections;
  std::shared_ptr<IPVRHbbTVEngine> m_engine;
};

} // namespace PVR
