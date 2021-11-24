/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonInstanceHandler.h"

#include "ServiceBroker.h"
#include "addons/settings/AddonSettings.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif

using XFILE::CDirectory;
using XFILE::CFile;

namespace ADDON
{

CCriticalSection IAddonInstanceHandler::m_cdSec;

IAddonInstanceHandler::IAddonInstanceHandler(ADDON_TYPE type,
                                             const AddonInfoPtr& addonInfo,
                                             KODI_HANDLE parentInstance /* = nullptr*/,
                                             const std::string& instanceID /* = ""*/)
  : m_type(type), m_parentInstance(parentInstance), m_addonInfo(addonInfo)
{
  // if no special instance ID is given generate one from class pointer (is
  // faster as unique id and also safe enough for them).
  m_instanceId = !instanceID.empty() ? instanceID : StringUtils::Format("{}", fmt::ptr(this));
  m_addonBase = CServiceBroker::GetBinaryAddonManager().GetAddonBase(addonInfo, this, m_addon);
}

IAddonInstanceHandler::IAddonInstanceHandler(ADDON_TYPE type,
                                             const AddonInfoPtr& addonInfo,
                                             const std::string& profilePath)
  : m_type(type), m_parentInstance(nullptr), m_addonInfo(addonInfo), m_profilePath(profilePath)
{
  // if no special instance ID is given generate one from class pointer (is
  // faster as unique id and also safe enough for them).
  m_instanceId = StringUtils::Format("%p", static_cast<void*>(this));
  m_addonBase = CServiceBroker::GetBinaryAddonManager().GetAddonBase(addonInfo, this, m_addon);

  if (!m_profilePath.empty())
    m_userSettingsPath = URIUtils::AddFileToFolder(m_profilePath, "settings.xml");
}

IAddonInstanceHandler::~IAddonInstanceHandler()
{
  CServiceBroker::GetBinaryAddonManager().ReleaseAddonBase(m_addonBase, this);
}

std::string IAddonInstanceHandler::ID() const
{
  return m_addon ? m_addon->ID() : "";
}

std::string IAddonInstanceHandler::Name() const
{
  return m_addon ? m_addon->Name() : "";
}

std::string IAddonInstanceHandler::Author() const
{
  return m_addon ? m_addon->Author() : "";
}

std::string IAddonInstanceHandler::Icon() const
{
  return m_addon ? m_addon->Icon() : "";
}

std::string IAddonInstanceHandler::Path() const
{
  return m_addon ? m_addon->Path() : "";
}

std::string IAddonInstanceHandler::Profile() const
{
  return m_profilePath.empty() ? m_addon ? m_addon->Profile() : "" : m_profilePath;
}

AddonVersion IAddonInstanceHandler::Version() const
{
  return m_addon ? m_addon->Version() : AddonVersion();
}

ADDON_STATUS IAddonInstanceHandler::CreateInstance(KODI_HANDLE instance)
{
  if (!m_addon)
    return ADDON_STATUS_UNKNOWN;

  CSingleLock lock(m_cdSec);

  ADDON_STATUS status =
      m_addon->CreateInstance(m_type, this, m_instanceId, instance, m_parentInstance);
  if (status != ADDON_STATUS_OK)
  {
    CLog::Log(LOGERROR,
              "IAddonInstanceHandler::{}: {} returned bad status \"{}\" during instance creation",
              __FUNCTION__, m_addon->ID(), kodi::TranslateAddonStatus(status));
  }
  return status;
}

void IAddonInstanceHandler::DestroyInstance()
{
  CSingleLock lock(m_cdSec);
  if (m_addon)
    m_addon->DestroyInstance(this);
}

/**
 * Settings Handling
 */
bool IAddonInstanceHandler::HasSettings()
{
  return LoadSettings(false) && m_settings->HasSettings();
}

bool IAddonInstanceHandler::SettingsInitialized() const
{
  return m_settings != nullptr && m_settings->IsInitialized();
}

bool IAddonInstanceHandler::SettingsLoaded() const
{
  return m_settings != nullptr && m_settings->IsLoaded();
}

bool IAddonInstanceHandler::LoadSettings(bool bForce, bool loadUserSettings /* = true */)
{
  if (SettingsInitialized() && !bForce)
    return true;

  if (m_loadSettingsFailed)
    return false;

  // assume loading settings fails
  m_loadSettingsFailed = true;

  // reset the settings if we are forced to
  if (SettingsInitialized() && bForce)
    GetSettings()->Uninitialize();

  // load the settings definition XML file
  auto addonSettingsDefinitionFile =
      URIUtils::AddFileToFolder(m_addonInfo->Path(), "resources", "settings.xml");
  CXBMCTinyXML addonSettingsDefinitionDoc;
  if (!addonSettingsDefinitionDoc.LoadFile(addonSettingsDefinitionFile))
  {
    if (CFile::Exists(addonSettingsDefinitionFile))
    {
      CLog::Log(LOGERROR, "CAddon[%s]: unable to load: %s, Line %d\n%s", ID().c_str(),
                addonSettingsDefinitionFile.c_str(), addonSettingsDefinitionDoc.ErrorRow(),
                addonSettingsDefinitionDoc.ErrorDesc());
    }

    return false;
  }

  // initialize the settings definition
  if (!GetSettings()->Initialize(addonSettingsDefinitionDoc))
  {
    CLog::Log(LOGERROR, "CAddon[%s]: failed to initialize addon settings", ID().c_str());
    return false;
  }

  // loading settings didn't fail
  m_loadSettingsFailed = false;

  // load user settings / values
  if (loadUserSettings)
    LoadUserSettings();

  return true;
}

bool IAddonInstanceHandler::HasUserSettings()
{
  if (!LoadSettings(false))
    return false;

  return SettingsLoaded() && m_hasUserSettings;
}

bool IAddonInstanceHandler::ReloadSettings()
{
  return LoadSettings(true);
}

void IAddonInstanceHandler::ResetSettings()
{
  m_settings.reset();
}

bool IAddonInstanceHandler::LoadUserSettings()
{
  if (!SettingsInitialized())
    return false;

  m_hasUserSettings = false;

  // there are no user settings
  if (!CFile::Exists(m_userSettingsPath))
  {
    // mark the settings as loaded
    GetSettings()->SetLoaded();
    return true;
  }

  CXBMCTinyXML doc;
  if (!doc.LoadFile(m_userSettingsPath))
  {
    CLog::Log(LOGERROR, "CAddon[%s]: failed to load addon settings from %s", ID().c_str(),
              m_userSettingsPath.c_str());
    return false;
  }

  return SettingsFromXML(doc);
}

bool IAddonInstanceHandler::HasSettingsToSave() const
{
  return SettingsLoaded();
}

void IAddonInstanceHandler::SaveSettings(void)
{
  if (!HasSettingsToSave())
    return; // no settings to save

  // break down the path into directories
  std::string strAddon = URIUtils::GetDirectory(m_userSettingsPath);
  std::string strRoot = URIUtils::GetDirectory(strAddon);

  // create the individual folders
  if (!CDirectory::Exists(strRoot))
    CDirectory::Create(strRoot);
  if (!CDirectory::Exists(strAddon))
    CDirectory::Create(strAddon);

  // create the XML file
  CXBMCTinyXML doc;
  if (SettingsToXML(doc))
    doc.SaveFile(m_userSettingsPath);

  m_hasUserSettings = true;

  //push the settings changes to the running addon instance
  CServiceBroker::GetAddonMgr().ReloadSettings(ID());
#ifdef HAS_PYTHON
  CServiceBroker::GetXBPython().OnSettingsChanged(ID());
#endif
}

std::string IAddonInstanceHandler::GetSetting(const std::string& key)
{
  if (key.empty() || !LoadSettings(false))
    return ""; // no settings available

  auto setting = m_settings->GetSetting(key);
  if (setting != nullptr)
    return setting->ToString();

  return "";
}

template<class TSetting>
bool GetSettingValue(IAddonInstanceHandler& addon,
                     const std::string& key,
                     typename TSetting::Value& value)
{
  if (key.empty() || !addon.HasSettings())
    return false;

  auto setting = addon.GetSettings()->GetSetting(key);
  if (setting == nullptr || setting->GetType() != TSetting::Type())
    return false;

  value = std::static_pointer_cast<TSetting>(setting)->GetValue();
  return true;
}

bool IAddonInstanceHandler::GetSettingBool(const std::string& key, bool& value)
{
  return GetSettingValue<CSettingBool>(*this, key, value);
}

bool IAddonInstanceHandler::GetSettingInt(const std::string& key, int& value)
{
  return GetSettingValue<CSettingInt>(*this, key, value);
}

bool IAddonInstanceHandler::GetSettingNumber(const std::string& key, double& value)
{
  return GetSettingValue<CSettingNumber>(*this, key, value);
}

bool IAddonInstanceHandler::GetSettingString(const std::string& key, std::string& value)
{
  return GetSettingValue<CSettingString>(*this, key, value);
}

void IAddonInstanceHandler::UpdateSetting(const std::string& key, const std::string& value)
{
  if (key.empty() || !LoadSettings(false))
    return;

  // try to get the setting
  auto setting = m_settings->GetSetting(key);

  // if the setting doesn't exist, try to add it
  if (setting == nullptr)
  {
    setting = m_settings->AddSetting(key, value);
    if (setting == nullptr)
    {
      CLog::Log(LOGERROR, "CAddon[%s]: failed to add undefined setting \"%s\"", ID().c_str(),
                key.c_str());
      return;
    }
  }

  setting->FromString(value);
}

template<class TSetting>
bool UpdateSettingValue(IAddonInstanceHandler& addon,
                        const std::string& key,
                        typename TSetting::Value value)
{
  if (key.empty() || !addon.HasSettings())
    return false;

  // try to get the setting
  auto setting = addon.GetSettings()->GetSetting(key);

  // if the setting doesn't exist, try to add it
  if (setting == nullptr)
  {
    setting = addon.GetSettings()->AddSetting(key, value);
    if (setting == nullptr)
    {
      CLog::Log(LOGERROR, "CAddon[%s]: failed to add undefined setting \"%s\"", addon.ID().c_str(),
                key.c_str());
      return false;
    }
  }

  if (setting->GetType() != TSetting::Type())
    return false;

  return std::static_pointer_cast<TSetting>(setting)->SetValue(value);
}

bool IAddonInstanceHandler::UpdateSettingBool(const std::string& key, bool value)
{
  return UpdateSettingValue<CSettingBool>(*this, key, value);
}

bool IAddonInstanceHandler::UpdateSettingInt(const std::string& key, int value)
{
  return UpdateSettingValue<CSettingInt>(*this, key, value);
}

bool IAddonInstanceHandler::UpdateSettingNumber(const std::string& key, double value)
{
  return UpdateSettingValue<CSettingNumber>(*this, key, value);
}

bool IAddonInstanceHandler::UpdateSettingString(const std::string& key, const std::string& value)
{
  return UpdateSettingValue<CSettingString>(*this, key, value);
}

bool IAddonInstanceHandler::SettingsFromXML(const CXBMCTinyXML& doc,
                                            bool loadDefaults /* = false */)
{
  if (doc.RootElement() == nullptr)
    return false;

  // if the settings haven't been initialized yet, try it from the given XML
  if (!SettingsInitialized())
  {
    if (!GetSettings()->Initialize(doc))
    {
      CLog::Log(LOGERROR, "CAddon[%s]: failed to initialize addon settings", ID().c_str());
      return false;
    }
  }

  // reset all setting values to their default value
  if (loadDefaults)
    GetSettings()->SetDefaults();

  // try to load the setting's values from the given XML
  if (!GetSettings()->Load(doc))
  {
    CLog::Log(LOGERROR, "CAddon[%s]: failed to load user settings", ID().c_str());
    return false;
  }

  m_hasUserSettings = true;

  return true;
}

bool IAddonInstanceHandler::SettingsToXML(CXBMCTinyXML& doc) const
{
  if (!SettingsInitialized())
    return false;

  if (!m_settings->Save(doc))
  {
    CLog::Log(LOGERROR, "CAddon[%s]: failed to save addon settings", ID().c_str());
    return false;
  }

  return true;
}

CAddonSettings* IAddonInstanceHandler::GetSettings() const
{
  // initialize addon settings if necessary
  if (m_settings == nullptr)
    m_settings = std::make_shared<CAddonSettings>(enable_shared_from_this::shared_from_this());

  return m_settings.get();
}

} /* namespace ADDON */

