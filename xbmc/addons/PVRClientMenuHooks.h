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

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct PVR_MENUHOOK;

namespace PVR
{
  class CPVRClientMenuHook
  {
  public:
    CPVRClientMenuHook() = delete;
    virtual ~CPVRClientMenuHook() = default;

    CPVRClientMenuHook(const std::string &addonId, const PVR_MENUHOOK &hook);

    bool IsAllHook() const;
    bool IsChannelHook() const;
    bool IsTimerHook() const;
    bool IsEpgHook() const;
    bool IsRecordingHook() const;
    bool IsDeletedRecordingHook() const;
    bool IsSettingsHook() const;

    unsigned int GetId() const;
    unsigned int GetLabelId() const;
    std::string GetLabel() const;

  private:
    std::string m_addonId;
    std::shared_ptr<PVR_MENUHOOK> m_hook;
  };

  class CPVRClientMenuHooks
  {
  public:
    CPVRClientMenuHooks() = default;
    virtual ~CPVRClientMenuHooks() = default;

    explicit CPVRClientMenuHooks(const std::string &addonId) : m_addonId(addonId) {}

    void AddHook(const PVR_MENUHOOK &hook);

    bool HasChannelHook() const;
    bool HasTimerHook() const;
    bool HasEpgHook() const;
    bool HasRecordingHook() const;
    bool HasDeletedRecordingHook() const;
    bool HasSettingsHook() const;

    std::vector<CPVRClientMenuHook> GetChannelHooks() const;
    std::vector<CPVRClientMenuHook> GetTimerHooks() const;
    std::vector<CPVRClientMenuHook> GetEpgHooks() const;
    std::vector<CPVRClientMenuHook> GetRecordingHooks() const;
    std::vector<CPVRClientMenuHook> GetDeletedRecordingHooks() const;
    std::vector<CPVRClientMenuHook> GetSettingsHooks() const;

  private:
    bool HasHook(std::function<bool(const CPVRClientMenuHook& hook)> function) const;
    std::vector<CPVRClientMenuHook> GetHooks(std::function<bool(const CPVRClientMenuHook& hook)> function) const;

    std::string m_addonId;
    std::unique_ptr<std::vector<CPVRClientMenuHook>> m_hooks;
  };

  typedef std::shared_ptr<CPVRClientMenuHooks> CPVRClientMenuHooksPtr;
}
