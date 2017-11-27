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

#include "PVRClientMenuHooks.h"

#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"

namespace PVR
{

CPVRClientMenuHook::CPVRClientMenuHook(const std::string &addonId, const PVR_MENUHOOK &hook)
: m_addonId(addonId),
  m_hook(new PVR_MENUHOOK(hook))
{
  if (hook.category != PVR_MENUHOOK_UNKNOWN &&
      hook.category != PVR_MENUHOOK_ALL &&
      hook.category != PVR_MENUHOOK_CHANNEL &&
      hook.category != PVR_MENUHOOK_TIMER &&
      hook.category != PVR_MENUHOOK_EPG &&
      hook.category != PVR_MENUHOOK_RECORDING &&
      hook.category != PVR_MENUHOOK_RECORDING &&
      hook.category != PVR_MENUHOOK_SETTING)
    CLog::Log(LOGERROR, "CPVRClientMenuHook - unknown PVR_MENUHOOK_CAT value: %d", hook.category);
}

bool CPVRClientMenuHook::IsAllHook() const
{
  return m_hook->category == PVR_MENUHOOK_ALL;
}

bool CPVRClientMenuHook::IsChannelHook() const
{
  return m_hook->category == PVR_MENUHOOK_CHANNEL;
}

bool CPVRClientMenuHook::IsTimerHook() const
{
  return m_hook->category == PVR_MENUHOOK_TIMER;
}

bool CPVRClientMenuHook::IsEpgHook() const
{
  return m_hook->category == PVR_MENUHOOK_EPG;
}

bool CPVRClientMenuHook::IsRecordingHook() const
{
  return m_hook->category == PVR_MENUHOOK_RECORDING;
}

bool CPVRClientMenuHook::IsDeletedRecordingHook() const
{
  return m_hook->category == PVR_MENUHOOK_DELETED_RECORDING;
}

bool CPVRClientMenuHook::IsSettingsHook() const
{
  return m_hook->category == PVR_MENUHOOK_SETTING;
}

unsigned int CPVRClientMenuHook::GetId() const
{
  return m_hook->iHookId;
}

unsigned int CPVRClientMenuHook::GetLabelId() const
{
  return m_hook->iLocalizedStringId;
}

std::string CPVRClientMenuHook::GetLabel() const
{
  return g_localizeStrings.GetAddonString(m_addonId, m_hook->iLocalizedStringId);
}

void CPVRClientMenuHooks::AddHook(const PVR_MENUHOOK &hook)
{
  if (!m_hooks)
    m_hooks.reset(new std::vector<CPVRClientMenuHook>());

  m_hooks->emplace_back(CPVRClientMenuHook(m_addonId, hook));
}

bool CPVRClientMenuHooks::HasHook(std::function<bool(const CPVRClientMenuHook& hook)> function) const
{
  if (!m_hooks)
    return false;

  for (const CPVRClientMenuHook& hook : *m_hooks)
  {
    if (function(hook) || hook.IsAllHook())
      return true;
  }
  return false;
}

bool CPVRClientMenuHooks::HasChannelHook() const
{
  return HasHook([](const CPVRClientMenuHook& hook)
  {
    return hook.IsChannelHook();
  });
}

bool CPVRClientMenuHooks::HasTimerHook() const
{
  return HasHook([](const CPVRClientMenuHook& hook)
  {
    return hook.IsTimerHook();
  });
}

bool CPVRClientMenuHooks::HasEpgHook() const
{
  return HasHook([](const CPVRClientMenuHook& hook)
  {
    return hook.IsEpgHook();
  });
}

bool CPVRClientMenuHooks::HasRecordingHook() const
{
  return HasHook([](const CPVRClientMenuHook& hook)
  {
    return hook.IsRecordingHook();
  });
}

bool CPVRClientMenuHooks::HasDeletedRecordingHook() const
{
  return HasHook([](const CPVRClientMenuHook& hook)
  {
    return hook.IsDeletedRecordingHook();
  });
}

bool CPVRClientMenuHooks::HasSettingsHook() const
{
  return HasHook([](const CPVRClientMenuHook& hook)
  {
    return hook.IsSettingsHook();
  });
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetHooks(std::function<bool(const CPVRClientMenuHook& hook)> function) const
{
  std::vector<CPVRClientMenuHook> hooks;

  if (!m_hooks)
    return hooks;

  for (const CPVRClientMenuHook& hook : *m_hooks)
  {
    if (function(hook) || hook.IsAllHook())
      hooks.emplace_back(hook);
  }
  return hooks;
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetChannelHooks() const
{
  return GetHooks([](const CPVRClientMenuHook& hook)
  {
    return hook.IsChannelHook();
  });
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetTimerHooks() const
{
  return GetHooks([](const CPVRClientMenuHook& hook)
  {
    return hook.IsTimerHook();
  });
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetEpgHooks() const
{
  return GetHooks([](const CPVRClientMenuHook& hook)
  {
    return hook.IsEpgHook();
  });
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetRecordingHooks() const
{
  return GetHooks([](const CPVRClientMenuHook& hook)
  {
    return hook.IsRecordingHook();
  });
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetDeletedRecordingHooks() const
{
  return GetHooks([](const CPVRClientMenuHook& hook)
  {
    return hook.IsDeletedRecordingHook();
  });
}

std::vector<CPVRClientMenuHook> CPVRClientMenuHooks::GetSettingsHooks() const
{
  return GetHooks([](const CPVRClientMenuHook& hook)
  {
    return hook.IsSettingsHook();
  });
}

} // namespace PVR
