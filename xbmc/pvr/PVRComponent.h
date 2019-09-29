/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CAdvancedSettings;
class CDatabase;
class DatabaseSettings;
class CFileItem;
class CFileItemList;
class CGUIViewState;
class CGUIWindowManager;

namespace ADDON
{
class CAddonInfo;
class IAddon;
}

namespace PVR
{

class CPVRComponent
{
public:
  /*!
   * @brief ctor.
   */
  CPVRComponent();

  /*!
   * @brief dtor.
   */
  virtual ~CPVRComponent();

  ////////////////////////////////////////////////////
  // services
  ////////////////////////////////////////////////////

  /*!
   * @brief Stop all PVR services.
   */
  void Stop();

  /*!
   * @brief Restart all PVR services.
   */
  void Restart();

  /*!
   * @brief Check whether PVR services are started.
   * @return Truf if started, false otherwise
   */
  bool IsStarted() const;

  ////////////////////////////////////////////////////
  // addons
  ////////////////////////////////////////////////////

  /*!
   * @brief Create a PVR client addon instance
   * @param info The addon info
   * @return The new instance
   */
  std::shared_ptr<ADDON::IAddon> CreatePVRAddonInstance(const std::shared_ptr<ADDON::CAddonInfo>& info);

  /*!
   * @brief Get the PVR client addon instance interafce from a given addon
   * @param addon The addon
   * @return The instance interface or nullptr
   */
  void* GetPVRAddonInstanceInterface(ADDON::IAddon* addon) const;

  ////////////////////////////////////////////////////
  // databases
  ////////////////////////////////////////////////////

  struct Database
  {
    std::shared_ptr<CDatabase> instance;
    DatabaseSettings* settings;
  };

  /*!
   * @brief Get all PVR databases
   * @param advancedSettings The advanced settings instance
   * @return The PVR databases.
   */
  std::vector<Database> GetDatabases(const std::shared_ptr<CAdvancedSettings>& advancedSettings) const;

  ////////////////////////////////////////////////////
  // gui lib
  ////////////////////////////////////////////////////

  /*!
   * @brief Create all PVR windows and dialogs and register with given window manager
   * @param guiManager The window manager
   */
  void CreateWindowsAndDialogs(CGUIWindowManager* guiManager);

  /*!
   * @brief Destroy all PVR windows and dialogs and deregister from given window manager
   * @param guiManager The window manager
   */
  void DestroyWindowsAndDialogs(CGUIWindowManager* guiManager);

  /*!
   * @brief Get fullscreen window ID valid for given item
   * @param guiManager The item
   * @return The window id or WINDOW_INVALID
   */
  int GetFullscreenWindowID(const CFileItem& item) const;

  /*!
   * @brief Get GUI view state for given window id and items
   * @param windowId The window id
   * @param items The items
   * @return The view state or nullptr if the given window id does not match any PVR window
   */
  CGUIViewState* GetViewState(int windowId, const CFileItemList& items) const;

  /*!
   * @brief Propagate change of GUI locale
   */
  void OnLocaleChanged();

  ////////////////////////////////////////////////////
  // power management
  ////////////////////////////////////////////////////

  /*!
   * @brief Propagate event on system sleep
   */
  void OnSleep();

  /*!
   * @brief Propagate event on system wake
   */
  void OnWake();

  /*!
   * @brief Check whether the system Kodi is running on can be powered down
   *        (shutdown/reboot/suspend/hibernate) without stopping any active
   *        recordings and/or without preventing the start of recordings
   *        scheduled for now + pvrpowermanagement.backendidletime.
   * @param bAskUser True to inform user in case of potential
   *        data loss. User can decide to allow powerdown anyway. False to
   *        not to ask user and to not confirm power down.
   * @return True if system can be safely powered down, false otherwise.
   */
  bool CanSystemPowerdown(bool bAskUser = true) const;

  ////////////////////////////////////////////////////
  // media playback
  ////////////////////////////////////////////////////

  /*!
   * @brief Start playback of the given file item.
   * @param item containing a channel or a recording.
   * @return True if the playback of the item could be started, false otherwise.
   */
  bool PlayMedia(const std::shared_ptr<CFileItem>& item) const;

  /*!
   * @brief Inform PVR component that playback of an item just started.
   * @param item The item that started to play.
   */
  void OnPlaybackStarted(const std::shared_ptr<CFileItem>& item);

  /*!
   * @brief Inform PVR component that playback of an item was stopped due to user interaction.
   * @param item The item that stopped to play.
   */
  void OnPlaybackStopped(const std::shared_ptr<CFileItem>& item);

  /*!
   * @brief Inform PVR component that playback of an item has stopped without user interaction.
   * @param item The item that ended to play.
   */
  void OnPlaybackEnded(const std::shared_ptr<CFileItem>& item);

  ////////////////////////////////////////////////////
  // channels
  ////////////////////////////////////////////////////

  /*!
   * @brief Check whether a given URI represents a PVR channel
   * @param uri The URI
   * @return True if URI represents a PVR channel, false otherwise
   */
  bool IsPVRChannel(const std::string& uri) const;

  ////////////////////////////////////////////////////
  // channel groups
  ////////////////////////////////////////////////////

  /*!
   * @brief Check whether a given URI represents a PVR channel group
   * @param uri The URI
   * @return True if URI represents a PVR channel group, false otherwise
   */
  bool IsPVRChannelGroup(const std::string& uri) const;

  ////////////////////////////////////////////////////
  // recordings
  ////////////////////////////////////////////////////

  /*!
   * @brief Reset a recording's resume point, if any
   * @param recording The recording
   * @return True on success, false otherwise
   */
  bool ResetRecordingResumePoint(const std::shared_ptr<CFileItem>& recording);

  /*!
   * @brief Set a recording's watched state
   * @param recording The recording
   * @param bWatched True to set watched, false to set unwatched state
   * @return True on success, false otherwise
   */
  bool MarkRecordingWatched(const std::shared_ptr<CFileItem>& recording, bool bWatched);
};

}
