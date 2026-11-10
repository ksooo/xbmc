/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRHbbTV.h"

#include "ServiceBroker.h"
#include "pvr/hbbtv/AITParser.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#include <chrono>
#include <mutex>

using namespace PVR;

namespace
{
// Loading an application while playback settles (refresh rate switch, buffering, A/V sync)
// makes the picture stutter on slow devices.
constexpr auto AUTOSTART_DELAY = std::chrono::seconds(5);
} // namespace

void CPVRHbbTV::OnSection(const CAITSection& section)
{
  if (!section.currentNext)
    return;

  {
    std::unique_lock lock(m_critSection);
    if (m_version != section.version)
    {
      m_sections.clear();
      m_version = section.version;
    }
    m_sections[section.sectionNumber] = section.applications;
  }

  for (const auto& app : section.applications)
    CLog::LogFC(LOGDEBUG, LOGPVR, "HbbTV application org {:#010x} id {:#06x} control {} '{}' {}",
                app.organisationId, app.applicationId, app.controlCode, app.name, app.url);

  if (!section.applications.empty())
    PrepareEngine(false);

  // The autostart application may arrive with a later section of the table.
  ScheduleAutostart();
}

void CPVRHbbTV::PrepareEngine(bool wait)
{
  // Starting the engine costs seconds of CPU time on slow devices. Doing it while a channel
  // starts playing makes the picture stutter, so warm it up before.
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_PVRPLAYBACK_ENABLEHBBTV))
    return;

  if (const auto engine = GetEngine())
    engine->Prepare(wait);
}

void CPVRHbbTV::OnPlaybackStarted()
{
  std::unique_lock lock(m_critSection);
  m_avStarted = false;
}

void CPVRHbbTV::OnAVStarted()
{
  {
    std::unique_lock lock(m_critSection);
    m_avStarted = true;
  }
  ScheduleAutostart();
}

void CPVRHbbTV::ScheduleAutostart()
{
  {
    std::unique_lock lock(m_critSection);
    if (m_autostartDone || !m_avStarted || m_autostartTimer.IsRunning())
      return;
  }
  if (IsApplicationRunning() || !GetAutostartApplication() ||
      !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_PVRPLAYBACK_AUTOSTARTHBBTV))
    return;

  m_autostartTimer.Start(AUTOSTART_DELAY);
}

void CPVRHbbTV::OnAutostartTimer()
{
  {
    std::unique_lock lock(m_critSection);
    if (m_autostartDone)
      return;
    m_autostartDone = true;
  }
  // The user may have started an application meanwhile.
  if (!IsApplicationRunning())
    LaunchAutostartApplication();
}

void CPVRHbbTV::Clear()
{
  m_autostartTimer.Stop();
  {
    std::unique_lock lock(m_critSection);
    m_version.reset();
    m_sections.clear();
    m_autostartDone = false;
  }
  CloseApplication();
}

void CPVRHbbTV::SetEngine(std::shared_ptr<IPVRHbbTVEngine> engine)
{
  std::unique_lock lock(m_critSection);
  m_engine = std::move(engine);
}

std::shared_ptr<IPVRHbbTVEngine> CPVRHbbTV::GetEngine() const
{
  std::unique_lock lock(m_critSection);
  return m_engine;
}

bool CPVRHbbTV::HasApplications() const
{
  std::unique_lock lock(m_critSection);
  for (const auto& [number, apps] : m_sections)
  {
    if (!apps.empty())
      return true;
  }
  return false;
}

std::vector<CPVRHbbTVApplication> CPVRHbbTV::GetApplications() const
{
  std::unique_lock lock(m_critSection);
  std::vector<CPVRHbbTVApplication> result;
  for (const auto& [number, apps] : m_sections)
    result.insert(result.end(), apps.begin(), apps.end());
  return result;
}

std::optional<CPVRHbbTVApplication> CPVRHbbTV::GetAutostartApplication() const
{
  std::optional<CPVRHbbTVApplication> result;
  for (const auto& app : GetApplications())
  {
    if (app.controlCode != static_cast<uint8_t>(HbbTVControlCode::AUTOSTART) || app.url.empty())
      continue;
    if (!result || app.priority > result->priority)
      result = app;
  }
  return result;
}

bool CPVRHbbTV::LaunchApplication(const std::string& url)
{
  const auto engine = GetEngine();
  if (!engine)
  {
    CLog::LogFC(LOGWARNING, LOGPVR, "HbbTV applications are not supported on this platform");
    return false;
  }
  engine->Launch(url);
  return true;
}

bool CPVRHbbTV::LaunchAutostartApplication()
{
  const auto app = GetAutostartApplication();
  if (!app)
  {
    CLog::LogFC(LOGINFO, LOGPVR, "The playing channel signals no HbbTV autostart application");
    return false;
  }
  CLog::LogFC(LOGINFO, LOGPVR, "Starting HbbTV application '{}'", app->name);
  return LaunchApplication(app->url);
}

void CPVRHbbTV::CloseApplication()
{
  if (const auto engine = GetEngine())
    engine->Close();
}

bool CPVRHbbTV::IsApplicationRunning() const
{
  const auto engine = GetEngine();
  return engine && engine->IsRunning();
}
