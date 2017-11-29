#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <functional>
#include <map>
#include <memory>
#include <vector>

// TODO(): leaking c-api types
// struct PVR_STREAM_PROPERTIES
// struct PVR_STREAM_TIMES
// struct PVR_EDL_ENTRY
// enum PVR_CONNECTION_STATE
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"

#include "addons/AddonManager.h"
#include "addons/PVRClientCapabilities.h"
#include "addons/PVRClientMenuHooks.h"
#include "threads/CriticalSection.h"

#include "pvr/PVRTypes.h"


namespace ADDON
{
  struct AddonEvent;
}

namespace PVR
{
  class CPVRChannelGroups;
  class CPVRChannelGroupInternal;

  class CPVRClient;
  typedef std::shared_ptr<CPVRClient> CPVRClientPtr;
  typedef std::map<int, CPVRClientPtr> CPVRClientMap;

  class CPVRTimersContainer;
  typedef std::vector<CPVRTimerTypePtr> CPVRTimerTypes;

  // TODO(): leaking c-api type
  typedef std::map<int, PVR_STREAM_PROPERTIES> STREAMPROPS;

  /**
   * Holds generic data about a backend (number of channels etc.)
   */
  struct SBackend
  {
    std::string name;
    std::string version;
    std::string host;
    int         numTimers = 0;
    int         numRecordings = 0;
    int         numDeletedRecordings = 0;
    int         numChannels = 0;
    long long   diskUsed = 0;
    long long   diskTotal = 0;
  };

  class CPVRClients : public ADDON::IAddonMgrCallback
  {
  public:
    CPVRClients(void);
    ~CPVRClients(void) override;

    /*!
     * @brief Start all clients.
     */
    void Start(void);

    /*!
     * @brief Stop all clients.
     */
    void Stop();

    /*!
     * @brief Continue all clients.
     */
    void Continue();

    /*!
     * @brief Update add-ons from the AddonManager
     * @param changedAddonId The id of the changed addon, empty string denotes 'any addon'.
     */
    void UpdateAddons(const std::string &changedAddonId = "");

    /*!
     * @brief Restart a single client add-on.
     * @param addon The add-on to restart.
     * @param bDataChanged True if the client's data changed, false otherwise (unused).
     * @return True if the client was found and restarted, false otherwise.
     */
    bool RequestRestart(ADDON::AddonPtr addon, bool bDataChanged) override;

    /*!
     * @brief Stop a client.
     * @param addon The client to stop.
     * @param bRestart If true, restart the client.
     * @return True if the client was found, false otherwise.
     */
    bool StopClient(const ADDON::AddonPtr &addon, bool bRestart);

    /*!
     * @brief Handle addon events (enable, disable, ...).
     * @param event The addon event.
     */
    void OnAddonEvent(const ADDON::AddonEvent& event);

    /*!
     * @brief Get a client given its ID.
     * @param strId The ID of the client.
     * @param addon On success, filled with the client matching the given ID, null otherwise.
     * @return True if the client was found, false otherwise.
     */
    bool GetClient(const std::string &strId, ADDON::AddonPtr &addon) const;

    /*!
     * @brief Get a client's numeric ID given its string ID.
     * @param strId The string ID.
     * @return The numeric ID matching the given string ID, -1 on error.
     */
    int GetClientId(const std::string& strId) const;

    /*!
     * @brief Get the number of created clients.
     * @return The amount of created clients.
     */
    int CreatedClientAmount(void) const;

    /*!
     * @brief Check whether there are any created clients.
     * @return True if at least one client is created.
     */
    bool HasCreatedClients(void) const;

    /*!
     * @brief Check whether a given client ID points to a created client.
     * @param iClientId The client ID.
     * @return True if the the client ID represents a created client, false otherwise.
     */
    bool IsCreatedClient(int iClientId) const;

    /*!
     * @brief Get the instance of the client, if it's created.
     * @param iClientId The ID of the client to get.
     * @param addon Will be filled with requested client on success, null otherwise.
     * @return True on success, false otherwise.
     */
    bool GetCreatedClient(int iClientId, CPVRClientPtr &addon) const;

    /*!
     * @brief Get all created clients.
     * @param clients All created clients will be added to this map.
     * @return The amount of clients added to the map.
     */
    int GetCreatedClients(CPVRClientMap &clients) const;

    /*!
     * @brief Get the ID of the first created client.
     * @return the ID or -1 if no clients are created;
     */
    int GetFirstCreatedClientID(void);

    /*!
     * @brief Get the number of enabled clients.
     * @return The amount of enabled clients.
     */
    int EnabledClientAmount(void) const;

    /*!
     * @brief Get the friendly name for the client with the given id.
     * @param iClientId The id of the client.
     * @param strName The friendly name of the client or an empty string when it wasn't found.
     * @return True if the client was found, false otherwise.
     */
    bool GetClientFriendlyName(int iClientId, std::string &strName) const;

    /*!
     * @brief Get the addon name for the client with the given id.
     * @param iClientId The id of the client.
     * @param strName The addon name of the client or an empty string when it wasn't found.
     * @return True if the client was found, false otherwise.
     */
    bool GetClientAddonName(int iClientId, std::string &strName) const;

    /*!
     * @brief Get the addon icon for the client with the given id.
     * @param iClientId The id of the client.
     * @param strIcon The path to the addon icon of the client or an empty string when it wasn't found.
     * @return True if the client was found, false otherwise.
     */
    bool GetClientAddonIcon(int iClientId, std::string &strIcon) const;

    /*!
     * Get the add-on ID of the client
     * @param iClientId The db id of the client
     * @return The add-on id
     */
    std::string GetClientAddonId(int iClientId) const;

    /*!
     * @brief Check if a client is currently playing a stream.
     * @return True if a stream (TV/radio channel, recording, epg event) is playing, false otherwise.
     */
    bool IsPlaying(void) const;

    /*!
     * @brief Check if a client is currently playing a TV channel.
     * @return True if a TV channel is playing, false otherwise.
     */
    bool IsPlayingTV(void) const;

    /*!
     * @brief Check if a client is currently playing a radio channel.
     * @return True if a radio channel playing, false otherwise.
     */
    bool IsPlayingRadio(void) const;

    /*!
     * @brief Check if a client is currently playing a recording.
     * @return True if a recording is playing, false otherwise.
     */
    bool IsPlayingRecording(void) const;

    /*!
     * @brief Check if a client is currently playing an epg event.
     * @return True if an epg tag is playing, false otherwise.
     */
    bool IsPlayingEpgTag(void) const;

    /*!
     * @brief Check if a client is currently playing an encrypted channel.
     * @return True if there is a client playing a TV/radio channel and that channel is encrypted, false otherwise.
     */
    bool IsPlayingEncryptedChannel(void) const;

    /*!
     * @brief Get the instance of the playing client, if there is one.
     * @param client Will be filled with requested client on success, null otherwise.
     * @return True on success, false otherwise.
     */
    bool GetPlayingClient(CPVRClientPtr &client) const;

    /*!
     * @brief Get the ID of the playing client, if there is one.
     * @return The ID or -1 if no client is playing.
     */
    int GetPlayingClientID(void) const;

    /*!
     * @brief Get the name of the playing client, if there is one.
     * @return The name of the client or an empty string if nothing is playing.
     */
    const std::string GetPlayingClientName(void) const;

    /*!
     * @brief Set the channel that is currently playing.
     * @param channel The channel that is currently playing.
     */
    void SetPlayingChannel(const CPVRChannelPtr &channel);

    /*!
     * @brief Clear the channel that is currently playing, if any.
     */
    void ClearPlayingChannel();

    /*!
     * @brief Get the channel that is currently playing.
     * @return the channel that is currently playing, NULL otherwise.
     */
    CPVRChannelPtr GetPlayingChannel() const;

    /*!
     * @brief Set the recording that is currently playing.
     * @param recording The recording that is currently playing.
     */
    void SetPlayingRecording(const CPVRRecordingPtr &recording);

    /*!
     * @brief Clear the recording that is currently playing, if any.
     */
    void ClearPlayingRecording();

    /*!
     * @brief Get the recording that is currently playing.
     * @return The recording that is currently playing, NULL otherwise.
     */
    CPVRRecordingPtr GetPlayingRecording(void) const;

    /*!
     * @brief Check whether there is an active recording on the currenlyt playing channel.
     * @return True if there is a playing channel and there is an active recording on that channel, false otherwise.
     */
    bool IsRecordingOnPlayingChannel(void) const;

    /*!
     * @brief Check whether the currently playing channel can be recorded instantly.
     * @return True if there is a playing channel that can be recorded instantly, false otherwise.
     */
    bool CanRecordInstantly(void);

    /*!
     * @brief Set the epg tag that is currently playing.
     * @param epgTag The tag that is currently playing.
     */
    void SetPlayingEpgTag(const CPVREpgInfoTagPtr &epgTag);

    /*!
     * @brief Clear the epg tag that is currently playing, if any.
     */
    void ClearPlayingEpgTag();

    /*!
     * @brief Get the epg tag that is currently playing.
     * @return The tag that is currently playing, NULL otherwise.
     */
    CPVREpgInfoTagPtr GetPlayingEpgTag(void) const;

    /*! @name general methods */
    //@{

    /*!
     * @brief Query the the given client's capabilities.
     * @param iClientId The client id.
     * @return The capabilities or null, if not found.
     */
    CPVRClientCapabilitiesPtr GetClientCapabilities(int iClientId) const;

    /*!
     * @brief Returns properties about all created clients
     * @return The properties
     */
    std::vector<SBackend> GetBackendProperties() const;

    /*!
     * @brief Returns the client's backend host name.
     * @return The host name or an empty string, if the client does not have a backend host.
     */
    std::string GetBackendHostnameByClientId(int iClientId) const;

    //@}

    /*! @name stream methods */
    //@{

    /*!
     * @brief Open a stream on the given channel.
     * @param channel The channel to start playing.
     * @return True if the stream was opened successfully, false otherwise.
     */
    bool OpenStream(const CPVRChannelPtr &channel);

    /*!
     * @brief Open a stream from the given recording.
     * @param recording The recording to start playing.
     * @return True if the stream was opened successfully, false otherwise.
     */
    bool OpenStream(const CPVRRecordingPtr &recording);

    /*!
     * @brief Close the stream on the currently playing client, if any.
     */
    void CloseStream(void);

    /*!
     * @brief Read from an open stream.
     * @param lpBuf Target buffer.
     * @param uiBufSize The size of the buffer.
     * @return The amount of bytes that was added or -1 in case of an error.
     */
    int ReadStream(void* lpBuf, int64_t uiBufSize);

    /*!
     * @brief Return the filesize of the currently playing stream.
     * @return The size of the stream or -1 in case of an error.
     */
    int64_t GetStreamLength(void);

    /*!
     * @brief Check whether it is possible to seek the currently playing livetv or recording stream.
     * @return True if there is a playing channel and if the playing stream can be seeked, false otherwise.
     */
    bool CanSeekStream(void) const;

    /*!
     * @brief Seek to a position in a stream.
     * @param iFilePosition The position to seek to.
     * @param iWhence Specify how to seek ("new position=pos", "new position=pos+actual position" or "new position=filesize-pos")
     * @return The new stream position or -1 in case of an error.
     */
    int64_t SeekStream(int64_t iFilePosition, int iWhence = SEEK_SET);

    /*!
     * @brief Check whether it is possible to pause the currently playing livetv or recording stream
     * @return True if there is a playing channel and if it can be paused, false otherwise.
     */
    bool CanPauseStream(void) const;

    /*!
     * @brief Pause/Continue a stream.
     * @param bPaused If true, pause the stream, unpause otherwise.
     */
    void PauseStream(bool bPaused);

    /*!
     * @brief Get the input format name of the current playing stream content.
     * @return A string containing the input format.
     */
    std::string GetCurrentInputFormat(void) const;

    /*!
     * @brief Fill the file item for a channel with the properties required for playback. Values are obtained from the PVR backend.
     * @param fileItem The file item to be filled.
     * @return True if the stream properties have been set, false otherwiese.
     */
    bool FillChannelStreamFileItem(CFileItem &fileItem);

    /*!
     * @brief Fill the file item for a recording with the properties required for playback. Values are obtained from the PVR backend.
     * @param fileItem The file item to be filled.
     * @return True if the stream properties have been set, false otherwiese.
     */
    bool FillRecordingStreamFileItem(CFileItem &fileItem);

    /*!
     * @brief Check whether the currently playing livetv stream is timeshifted.
     * @return True if there is a playing stream and if it is timeshifted, false otherwise.
     */
    bool IsTimeshifting() const;

    /*!
     * @brief Get the playing time of the currently playing stream.
     * @return The time.
     */
    time_t GetPlayingTime() const;

    /*!
     * @brief Get the start time of the timeshift buffer for the currently playing stream.
     * @return The buffer start time.
     */
    time_t GetBufferTimeStart() const;

    /*!
     * @brief Get the end time of the timeshift buffer for the currently playing stream.
     * @return The buffer end time.
     */
    time_t GetBufferTimeEnd() const;

    /*!
     * @brief Get timing data for the currently playing stream.
     * @param times The struct the client has to fill with data.
     * @return True, if the data were fetched successfully, false itherwise.
     */
    // TODO(): leaking c-api type
    bool GetStreamTimes(PVR_STREAM_TIMES *times) const;

    /*!
     * @brief Check if the currently playing stream is a realtime stream.
     * @return True if there is a playing stream and if it is realtime, false otherwise.
     */
   bool IsRealTimeStream() const;

    //@}

    /*! @name Timer methods */
    //@{

    /*!
     * @brief Check whether there is at least one created client supporting timers.
     * @return True if at least one created client supports timers, false otherwise.
     */
    bool SupportsTimers() const;

    /*!
     * @brief Get all timers from all created clients
     * @param timers Store the timers in this container.
     * @param failedClients in case of errors will contain the ids of the clients for which the timers could not be obtained.
     * @return True on success for all clients, false in case of error for at least one client.
     */
    bool GetTimers(CPVRTimersContainer *timers, std::vector<int> &failedClients);

    /*!
     * @brief Add a new timer to a backend.
     * @param timer The timer to add.
     * @return True if the operation succeeded, false otherwise.
     */
    bool AddTimer(const CPVRTimerInfoTag &timer);

    /*!
     * @brief Update a timer on the backend.
     * @param timer The timer to update.
     * @return True if the operation succeeded, false otherwise.
     */
    bool UpdateTimer(const CPVRTimerInfoTag &timer);

    /*!
     * @brief Delete a timer from the backend.
     * @param timer The timer to delete.
     * @param bForce Also delete when currently recording if true.
     * @param bRunning If bForce is set to false and the timer is recording, bRunning will be set to true.
     * @return True if the operation succeeded, false otherwise.
     */
    bool DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce, bool &bRunning);

    /*!
     * @brief Get all supported timer types.
     * @param results The container to store the result in.
     * @return True if the operation succeeded, false otherwise.
     */
    bool GetTimerTypes(CPVRTimerTypes& results) const;

    /*!
     * @brief Get all timer types supported by a certain client.
     * @param results The container to store the result in.
     * @param iClientId The id of the client.
     * @return True if the operation succeeded, false otherwise.
     */
    bool GetTimerTypes(CPVRTimerTypes& results, int iClientId) const;

    //@}

    /*! @name Recording methods */
    //@{

    /*!
     * @brief Get all recordings from clients
     * @param recordings Store the recordings in this container.
     * @param deleted If true, return deleted recordings, return not deleted recordings otherwise.
     * @return True if the operation succeeded, false otherwise.
     */
    bool GetRecordings(CPVRRecordings *recordings, bool deleted);

    /*!
     * @brief Rename a recording on the backend.
     * @param recording The recording to rename.
     * @return True if the operation succeeded, false otherwise.
     */
    bool RenameRecording(const CPVRRecording &recording);

    /*!
     * @brief Delete a recording from the backend.
     * @param recording The recording to delete.
     * @return True if the operation succeeded, false otherwise.
     */
    bool DeleteRecording(const CPVRRecording &recording);

    /*!
     * @brief Undelete a recording from the backend.
     * @param recording The recording to undelete.
     * @return True if the operation succeeded, false otherwise.
     */
    bool UndeleteRecording(const CPVRRecording &recording);

    /*!
     * @brief Delete all "soft" deleted recordings permanently on the backend.
     * @return True if the operation succeeded, false otherwise.
     */
    bool DeleteAllRecordingsFromTrash();

    /*!
     * @brief Set the lifetime of a recording on the backend.
     * @param recording The recording to set the lifetime for. recording.m_iLifetime contains the new lifetime value.
     * @return True if the recording's lifetime was set successfully, false otherwise.
     */
    bool SetRecordingLifetime(const CPVRRecording &recording);

    /*!
     * @brief Set play count of a recording on the backend.
     * @param recording The recording to set the play count.
     * @param count Play count.
     * @return True if the recording's play count was set successfully, false otherwise.
     */
    bool SetRecordingPlayCount(const CPVRRecording &recording, int count);

    /*!
     * @brief Set the last watched position of a recording on the backend.
     * @param recording The recording.
     * @param position The last watched position in seconds
     * @return True if the last played position was updated successfully, false otherwise.
    */
    bool SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition);

    /*!
    * @brief Retrieve the last watched position of a recording on the backend.
    * @param recording The recording.
    * @return The last watched position in seconds.
    */
    int GetRecordingLastPlayedPosition(const CPVRRecording &recording);

    /*!
    * @brief Retrieve the edit decision list (EDL) for a given recording from the backend.
    * @param recording The recording.
    * @return The edit decision list (empty on error).
    */
    // TODO(): leaking c-api type
    std::vector<PVR_EDL_ENTRY> GetRecordingEdl(const CPVRRecording &recording);

    //@}

    /*! @name EPG methods */
    //@{

    /*!
     * @brief Get the EPG table for a channel.
     * @param channel The channel to get the EPG table for.
     * @param epg Store the EPG in this container.
     * @param start Get entries after this start time.
     * @param end Get entries before this end time.
     * @return True if the operation succeeded, false otherwise.
     */
    bool GetEPGForChannel(const CPVRChannelPtr &channel, CPVREpg *epg, time_t start, time_t end);

    /*!
     * Tell the client the time frame to use when notifying epg events back to Kodi. The client might push epg events asynchronously
     * to Kodi using the callback function EpgEventStateChange. To be able to only push events that are actually of interest for Kodi,
     * client needs to know about the epg time frame Kodi uses.
     * @param iDays number of days from "now". EPG_TIMEFRAME_UNLIMITED means that Kodi is interested in all epg events, regardless of event times.
     * @return True if the operation succeeded, false otherwise.
     */
    bool SetEPGTimeFrame(int iDays);

    /*
     * @brief Check if an epg tag can be recorded.
     * @param tag The epg tag.
     * @param bIsRecordable Set to true if the tag can be recorded.
     * @return True if the operation succeeded, false otherwise.
     */
    bool IsRecordable(const CConstPVREpgInfoTagPtr &tag, bool &bIsRecordable) const;

    /*
     * @brief Check if an epg tag can be played.
     * @param tag The epg tag.
     * @param bIsPlayable Set to true if the tag can be played.
     * @return True if the operation succeeded, false otherwise.
     */
    bool IsPlayable(const CConstPVREpgInfoTagPtr &tag, bool &bIsPlayable) const;

    /*!
     * @brief Fill the file item for an epg tag with the properties required for playback. Values are obtained from the PVR backend.
     * @param fileItem The file item to be filled.
     * @return True if the stream properties have been set, false otherwiese.
     */
    bool FillEpgTagStreamFileItem(CFileItem &fileItem);

    //@}

    /*! @name Channel methods */
    //@{

    /*!
     * @brief Get all channels from backends.
     * @param group The container to store the channels in.
     * @param failedClients in case of errors will contain the ids of the clients for which the channels could not be obtained.
     * @return True if the operation succeeded, false otherwise.
     */
    bool GetChannels(CPVRChannelGroupInternal *group, std::vector<int> &failedClients);

    /*!
     * @brief Get all channel groups from backends.
     * @param groups Store the channel groups in this container.
     * @param failedClients in case of errors will contain the ids of the clients for which the channel groups could not be obtained.
     * @return True if the operation succeeded, false otherwise.
     */
    bool GetChannelGroups(CPVRChannelGroups *groups, std::vector<int> &failedClients);

    /*!
     * @brief Get all group members of a channel group.
     * @param group The group to get the member for.
     * @param failedClients in case of errors will contain the ids of the clients for which the channel group members could not be obtained.
     * @return True if the operation succeeded, false otherwise.
     */
    bool GetChannelGroupMembers(CPVRChannelGroup *group, std::vector<int> &failedClients);

    /*!
     * @brief Delete a channel on the backend.
     * @param channel The channel to delete.
     * @return True if the operation succeeded, false otherwise.
     */
    bool DeleteChannel(const CPVRChannelPtr &channel);

    /*!
     * @brief Rename the given channel on the backend.
     * @param channel The channel to rename.
     * @return True if the operation was successful, false otherwise.
     */
    bool RenameChannel(const CPVRChannelPtr &channel);

    /*!
     * @brief Get a list of clients providing a channel scan dialog.
     * @return All clients supporting channel scan.
     */
    std::vector<CPVRClientPtr> GetClientsSupportingChannelScan(void) const;

    /*!
     * @brief Get a list of clients providing a channel settings dialog.
     * @return All clients supporting channel settings.
     */
    std::vector<CPVRClientPtr> GetClientsSupportingChannelSettings(bool bRadio) const;

    /*!
     * @brief Open addon settings dialog to add a channel.
     * @param channel The channel to add.
     * @return True if the operation succeeded, false otherwise.
     */
    bool OpenDialogChannelAdd(const CPVRChannelPtr &channel);

    /*!
     * @brief Open addon channel settings dialog to edit channel properties.
     * @param channel The channel to edit.
     * @return True if the operation succeeded, false otherwise.
     */
    bool OpenDialogChannelSettings(const CPVRChannelPtr &channel);

    //@}

    /*! @name Menu hook methods */
    //@{

    /*!
     * @brief Get a client's menu hooks.
     * @return The hooks. Guaranteed never to be null.
     * @param iClientId The ID of the client to get the menu hooks for.
     * @return The hooks or null, if the client could not be found.
     */
    CPVRClientMenuHooksPtr GetMenuHooks(int iClientId) const;

    /*!
     * @brief Call one of the menu hooks of this client.
     * @param iClientId The ID of the client to call the menu hooks for.
     * @param hook The hook to call.
     * @param item The item for which the hook shall be called.
     * @return True on success, false otherwise.
     */
    bool CallMenuHook(int iClientId, const CPVRClientMenuHook &hook, const CFileItemPtr &item);

    //@}

    /*! @name Power management methods */
    //@{

    /*!
     * @brief Propagate "system sleep" event to clients
     */
    void OnSystemSleep();

    /*!
     * @brief Propagate "system wakup" event to clients
     */
    void OnSystemWake();

    /*!
     * @brief Propagate "power saving activated" event to clients
     */
    void OnPowerSavingActivated();

    /*!
     * @brief Propagate "power saving deactivated" event to clients
     */
    void OnPowerSavingDeactivated();

    /*!
     * @brief Get the priority for a given client. Larger value means higher priority.
     * @param iClientId The ID of the client to get the priority for.
     * @return The priority or -1 if the client could not be found.
     */
    int GetPriority(int iClientId) const;

    //@}

    /*!
     * @brief Notify a change of an addon connection state.
     * @param client The changed client.
     * @param strConnectionString A human-readable string identifiying the addon.
     * @param newState The new connection state.
     * @param strMessage A human readable message providing additional information.
     */
    // TODO(): leaking c-api type
    void ConnectionStateChange(CPVRClient *client, std::string &strConnectionString, PVR_CONNECTION_STATE newState, std::string &strMessage);

  private:
    /*!
     * @brief Get the client instance for a given client id.
     * @param iClientId The id of the client to get.
     * @param addon The client.
     * @return True if the client was found, false otherwise.
     */
    bool GetClient(int iClientId, CPVRClientPtr &addon) const;

    /*!
     * @brief Check whether a client is known.
     * @param client The client to check.
     * @return True if this client is known, false otherwise.
     */
    bool IsKnownClient(const ADDON::AddonPtr &client) const;

    /*!
     * @brief Check whether an given addon instance is a created pvr client.
     * @param addon The addon.
     * @return True if the the addon represents a created client, false otherwise.
     */
    bool IsCreatedClient(const ADDON::AddonPtr &addon);

    /*!
     * @brief Get all created clients and clients not (yet) ready to use.
     * @param clientsReady Store the created clients in this map.
     * @param clientsNotReady Store the the ids of the not (yet) ready clients in this list.
     * @return True if the operation succeeded, false otherwise.
     */
    bool GetCreatedClients(CPVRClientMap &clientsReady, std::vector<int> &clientsNotReady) const;

    typedef std::function<bool(const CPVRClientPtr&)> PVRClientFunction;

    /*!
     * @brief Wraps calls to all created clients in order to do common pre and post function invocation actions.
     * @param strFunctionName The function name, for logging purposes.
     * @param function The function to wrap. It has to have return type PVR_ERROR and must take a const reference to a CPVRClientPtr as parameter.
     * @return PVR_ERROR_NO_ERROR on success, any other PVR_ERROR_* value otherwise.
     */
    bool ForCreatedClients(const char* strFunctionName, PVRClientFunction function) const;

    /*!
     * @brief Wraps calls to all created clients in order to do common pre and post function invocation actions.
     * @param strFunctionName The function name, for logging purposes.
     * @param function The function to wrap. It has to have return type PVR_ERROR and must take a const reference to a CPVRClientPtr as parameter.
     * @param failedClients Contains a list of the ids of clients for that the call failed, if any.
     * @return PVR_ERROR_NO_ERROR on success, any other PVR_ERROR_* value otherwise.
     */
    bool ForCreatedClients(const char* strFunctionName, PVRClientFunction function, std::vector<int> &failedClients) const;

    /*!
     * @brief Wraps a call to a created client in order to do common pre and post function invocation actions.
     * @param strFunctionName The function name, for logging purposes.
     * @param iClientId The id of the client to call.
     * @param function The function to wrap. It has to have return type PVR_ERROR and must take a const reference to a CPVRClientPtr as parameter.
     * @return PVR_ERROR_NO_ERROR on success, any other PVR_ERROR_* value otherwise.
     */
    bool ForCreatedClient(const char* strFunctionName, int iClientId, PVRClientFunction function) const;

    /*!
     * @brief Wraps a call to the playing client, if any, in order to do common pre and post function invocation actions.
     * @param strFunctionName The function name, for logging purposes.
     * @param function The function to wrap. It has to have return type PVR_ERROR and must take a const reference to a CPVRClientPtr as parameter.
     * @return PVR_ERROR_NO_ERROR on success, PVR_ERROR_REJECTED if there is no playing client, any other PVR_ERROR_* value otherwise.
     */
    bool ForPlayingClient(const char* strFunctionName, PVRClientFunction function) const;

    int                   m_playingClientId;          /*!< the ID of the client that is currently playing */
    bool                  m_bIsPlayingLiveTV;
    bool                  m_bIsPlayingRecording;
    bool                  m_bIsPlayingEpgTag;
    std::string           m_strPlayingClientName;     /*!< the name client that is currently playing a stream or an empty string if nothing is playing */
    CPVRClientMap         m_clientMap;                /*!< a map of all known clients */
    CCriticalSection      m_critSection;
    std::map<std::string, int> m_addonNameIds; /*!< map add-on names to IDs */
  };
}
