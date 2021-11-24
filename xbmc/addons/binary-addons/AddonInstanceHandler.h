/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonDll.h"
#include "addons/AddonVersion.h"
#include "addons/kodi-dev-kit/include/kodi/AddonBase.h"

#include <memory>

namespace ADDON
{
class CAddonSettings;

class IAddonInstanceHandler : public std::enable_shared_from_this<IAddonInstanceHandler>
{
public:
  IAddonInstanceHandler(ADDON_TYPE type,
                        const AddonInfoPtr& addonInfo,
                        KODI_HANDLE parentInstance = nullptr,
                        const std::string& instanceID = "");
  IAddonInstanceHandler(ADDON_TYPE type,
                        const AddonInfoPtr& addonInfo,
                        const std::string& profilePath);
  virtual ~IAddonInstanceHandler();

  ADDON_TYPE UsedType() const { return m_type; }
  const std::string& InstanceID() { return m_instanceId; }

  std::string ID() const;
  std::string Name() const;
  std::string Author() const;
  std::string Icon() const;
  std::string Path() const;
  std::string Profile() const;
  AddonVersion Version() const;

  ADDON_STATUS CreateInstance(KODI_HANDLE instance);
  void DestroyInstance();
  const AddonDllPtr& Addon() const { return m_addon; }
  AddonInfoPtr GetAddonInfo() const { return m_addonInfo; };

  virtual void OnPreInstall() {}
  virtual void OnPostInstall(bool update, bool modal) {}
  virtual void OnPreUnInstall() {}
  virtual void OnPostUnInstall() {}

  CAddonSettings* GetSettings() const;

  bool HasSettings();
  bool HasUserSettings();
  void SaveSettings();
  void UpdateSetting(const std::string& key, const std::string& value);
  bool UpdateSettingBool(const std::string& key, bool value);
  bool UpdateSettingInt(const std::string& key, int value);
  bool UpdateSettingNumber(const std::string& key, double value);
  bool UpdateSettingString(const std::string& key, const std::string& value);
  std::string GetSetting(const std::string& key);
  bool GetSettingBool(const std::string& key, bool& value);
  bool GetSettingInt(const std::string& key, int& value);
  bool GetSettingNumber(const std::string& key, double& value);
  bool GetSettingString(const std::string& key, std::string& value);
  bool ReloadSettings();
  void ResetSettings();

private:
  bool SettingsInitialized() const;
  bool SettingsLoaded() const;
  bool LoadSettings(bool bForce, bool loadUserSettings = true);
  bool LoadUserSettings();
  bool HasSettingsToSave() const;
  bool SettingsFromXML(const CXBMCTinyXML& doc, bool loadDefaults = false);
  bool SettingsToXML(CXBMCTinyXML& doc) const;

  ADDON_TYPE m_type;
  std::string m_instanceId;
  KODI_HANDLE m_parentInstance;
  AddonInfoPtr m_addonInfo;
  std::string m_profilePath;
  std::string m_userSettingsPath;
  mutable std::shared_ptr<CAddonSettings> m_settings;
  bool m_loadSettingsFailed = false;
  bool m_hasUserSettings = false;
  BinaryAddonBasePtr m_addonBase;
  AddonDllPtr m_addon;

  static CCriticalSection m_cdSec;
};

} /* namespace ADDON */
