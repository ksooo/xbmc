/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "addons/binary-addons/AddonDll.h"

// PVR Addon API types
struct EPG_TAG;
struct PVR_CHANNEL;
struct PVR_CHANNEL_GROUP;
struct PVR_CHANNEL_GROUP_MEMBER;
struct PVR_DESCRAMBLE_INFO;
struct PVR_EDL_ENTRY;
struct PVR_MENUHOOK;
struct PVR_NAMED_VALUE;
struct PVR_RECORDING;
struct PVR_SIGNAL_STATUS;
struct PVR_STREAM_PROPERTIES;
struct PVR_STREAM_TIMES;
struct PVR_TIMER;

struct DemuxPacket;

struct AddonInstance_PVR;
struct KodiToAddonFuncTable_PVR;

namespace PVR
{
  class CPVRChannelGroup;
  class CPVRChannelGroups;
  class CPVRClientMenuHook;
  class CPVRClientCapabilities;
  class CPVRClientMenuHooks;
  class CPVREpg;
  class CPVRRecordings;
  class CPVRTimersContainer;
  class CPVRTimerType;

  #define PVR_INVALID_CLIENT_ID (-2)

  // Wrapper for Addon API enum PVR_ERROR
  enum class PVRClientError
  {
    NO_ERROR           = 0,  /*!< @brief no error occurred */
    UNKNOWN            = -1, /*!< @brief an unknown error occurred */
    NOT_IMPLEMENTED    = -2, /*!< @brief the method that Kodi called is not implemented by the add-on */
    SERVER_ERROR       = -3, /*!< @brief the backend reported an error, or the add-on isn't connected */
    SERVER_TIMEOUT     = -4, /*!< @brief the command was sent to the backend, but the response timed out */
    REJECTED           = -5, /*!< @brief the command was rejected by the backend */
    ALREADY_PRESENT    = -6, /*!< @brief the requested item can not be added, because it's already present */
    INVALID_PARAMETERS = -7, /*!< @brief the parameters of the method that was called are invalid for this operation */
    RECORDING_RUNNING  = -8, /*!< @brief a recording is running, so the timer can't be deleted without doing a forced delete */
    FAILED             = -9, /*!< @brief the command failed */
  };

  // Wrapper for Addon API enum PVR_CONNECTION_STATE
  enum class PVRClientConnectionState
  {
    UNKNOWN            = 0,  /*!< @brief unknown state (e.g. not yet tried to connect) */
    SERVER_UNREACHABLE = 1,  /*!< @brief backend server is not reachable (e.g. server not existing or network down)*/
    SERVER_MISMATCH    = 2,  /*!< @brief backend server is reachable, but there is not the expected type of server running (e.g. HTSP required, but FTP running at given server:port) */
    VERSION_MISMATCH   = 3,  /*!< @brief backend server is reachable, but server version does not match client requirements */
    ACCESS_DENIED      = 4,  /*!< @brief backend server is reachable, but denies client access (e.g. due to wrong credentials) */
    CONNECTED          = 5,  /*!< @brief connection to backend server is established */
    DISCONNECTED       = 6,  /*!< @brief no connection to backend server (e.g. due to network errors or client initiated disconnect)*/
    CONNECTING         = 7,  /*!< @brief connecting to backend */
  };

  class CPVRClient : public ADDON::CAddonDll
  {
  public:
    explicit CPVRClient(ADDON::CAddonInfo addonInfo);
    ~CPVRClient(void) override;

    void OnPreInstall() override;
    void OnPreUnInstall() override;
    ADDON::AddonPtr GetRunningInstance() const override;

    /** @name PVR add-on methods */
    //@{

    /*!
     * @brief Initialise the instance of this add-on.
     * @param iClientId The ID of this add-on.
     */
    ADDON_STATUS Create(int iClientId);

    /*!
     * @return True when the dll for this add-on was loaded, false otherwise (e.g. unresolved symbols)
     */
    bool DllLoaded(void) const;

    /*!
     * @brief Stop this add-on instance. No more client add-on access after this call.
     */
    void Stop();

    /*!
     * @brief Continue this add-on instance. Client add-on access is okay again after this call.
     */
    void Continue();

    /*!
     * @brief Destroy the instance of this add-on.
     */
    void Destroy(void);

    /*!
     * @brief Destroy and recreate this add-on.
     */
    void ReCreate(void);

    /*!
     * @return True if this instance is initialised (ADDON_Create returned true), false otherwise.
     */
    bool ReadyToUse(void) const;

    /*!
     * @brief Gets the backend connection state.
     * @return the backend connection state.
     */
    PVRClientConnectionState GetConnectionState(void) const;

    /*!
     * @brief Sets the backend connection state.
     * @param state the new backend connection state.
     */
    void SetConnectionState(PVRClientConnectionState state);

    /*!
     * @brief Gets the backend's previous connection state.
     * @return the backend's previous  connection state.
     */
    PVRClientConnectionState GetPreviousConnectionState(void) const;

    /*!
     * @brief signal to PVRManager this client should be ignored
     * @return true if this client should be ignored
     */
    bool IgnoreClient(void) const;

    /*!
     * @return The ID of this instance.
     */
    int GetID(void) const;

    //@}
    /** @name PVR server methods */
    //@{

    /*!
     * @brief Query this add-on's capabilities.
     * @return The add-on's capabilities.
     */
    std::shared_ptr<CPVRClientCapabilities> GetClientCapabilities(void) const { return m_clientCapabilities; }

    /*!
     * @brief Get the stream properties of the stream that's currently being read.
     * @param pProperties The properties.
     * @return PVR_ERROR_NO_ERROR if the properties have been fetched successfully.
     */
    PVRClientError GetStreamProperties(PVR_STREAM_PROPERTIES *pProperties);

    /*!
     * @return The name reported by the backend.
     */
    const std::string& GetBackendName(void) const;

    /*!
     * @return The version string reported by the backend.
     */
    const std::string& GetBackendVersion(void) const;

    /*!
     * @brief the ip address or alias of the pvr backend server
     */
    const std::string& GetBackendHostname(void) const;

    /*!
     * @return The connection string reported by the backend.
     */
    const std::string& GetConnectionString(void) const;

    /*!
     * @return A friendly name for this add-on that can be used in log messages.
     */
    const std::string& GetFriendlyName(void) const;

    /*!
     * @brief Get the disk space reported by the server.
     * @param iTotal The total disk space.
     * @param iUsed The used disk space.
     * @return PVR_ERROR_NO_ERROR if the drive space has been fetched successfully.
     */
    PVRClientError GetDriveSpace(long long &iTotal, long long &iUsed);

    /*!
     * @brief Start a channel scan on the server.
     * @return PVR_ERROR_NO_ERROR if the channel scan has been started successfully.
     */
    PVRClientError StartChannelScan(void);

    /*!
     * @brief Request the client to open dialog about given channel to add
     * @param channel The channel to add
     * @return PVR_ERROR_NO_ERROR if the add has been fetched successfully.
     */
    PVRClientError OpenDialogChannelAdd(const std::shared_ptr<CPVRChannel> &channel);

    /*!
     * @brief Request the client to open dialog about given channel settings
     * @param channel The channel to edit
     * @return PVR_ERROR_NO_ERROR if the edit has been fetched successfully.
     */
    PVRClientError OpenDialogChannelSettings(const std::shared_ptr<CPVRChannel> &channel);

    /*!
     * @brief Request the client to delete given channel
     * @param channel The channel to delete
     * @return PVR_ERROR_NO_ERROR if the delete has been fetched successfully.
     */
    PVRClientError DeleteChannel(const std::shared_ptr<CPVRChannel> &channel);

    /*!
     * @brief Request the client to rename given channel
     * @param channel The channel to rename
     * @return PVR_ERROR_NO_ERROR if the rename has been fetched successfully.
     */
    PVRClientError RenameChannel(const std::shared_ptr<CPVRChannel> &channel);

    /*
     * @brief Check if an epg tag can be recorded
     * @param tag The epg tag
     * @param bIsRecordable Set to true if the tag can be recorded
     * @return PVR_ERROR_NO_ERROR if bIsRecordable has been set successfully.
     */
    PVRClientError IsRecordable(const std::shared_ptr<const CPVREpgInfoTag> &tag, bool &bIsRecordable) const;

    /*
     * @brief Check if an epg tag can be played
     * @param tag The epg tag
     * @param bIsPlayable Set to true if the tag can be played
     * @return PVR_ERROR_NO_ERROR if bIsPlayable has been set successfully.
     */
    PVRClientError IsPlayable(const std::shared_ptr<const CPVREpgInfoTag> &tag, bool &bIsPlayable) const;

    /*!
     * @brief Fill the file item for an epg tag with the properties required for playback. Values are obtained from the PVR backend.
     * @param fileItem The file item to be filled.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError FillEpgTagStreamFileItem(CFileItem &fileItem);

    //@}
    /** @name PVR EPG methods */
    //@{

    /*!
     * @brief Request an EPG table for a channel from the client.
     * @param channel The channel to get the EPG table for.
     * @param epg The table to write the data to.
     * @param start The start time to use.
     * @param end The end time to use.
     * @param bSaveInDb If true, tell the callback method to save any new entry in the database or not. see CAddonCallbacksPVR::PVRTransferEpgEntry()
     * @return PVR_ERROR_NO_ERROR if the table has been fetched successfully.
     */
    PVRClientError GetEPGForChannel(const std::shared_ptr<CPVRChannel> &channel, CPVREpg *epg, time_t start = 0, time_t end = 0, bool bSaveInDb = false);

    /*!
     * Tell the client the time frame to use when notifying epg events back to Kodi. The client might push epg events asynchronously
     * to Kodi using the callback function EpgEventStateChange. To be able to only push events that are actually of interest for Kodi,
     * client needs to know about the epg time frame Kodi uses.
     * @param iDays number of days from "now". EPG_TIMEFRAME_UNLIMITED means that Kodi is interested in all epg events, regardless of event times.
     * @return PVR_ERROR_NO_ERROR if new value was successfully set.
     */
    PVRClientError SetEPGTimeFrame(int iDays);

    //@}
    /** @name PVR channel group methods */
    //@{

    /*!
     * @brief Get the total amount of channel groups from the backend.
     * @param iGroups The total amount of channel groups on the server or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError GetChannelGroupsAmount(int &iGroups);

    /*!
     * @brief Request the list of all channel groups from the backend.
     * @param groups The groups container to get the groups for.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVRClientError GetChannelGroups(CPVRChannelGroups *groups);

    /*!
     * @brief Request the list of all group members from the backend.
     * @param group The group to get the members for.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVRClientError GetChannelGroupMembers(CPVRChannelGroup *group);

    //@}
    /** @name PVR channel methods */
    //@{

    /*!
     * @brief Get the total amount of channels from the backend.
     * @param iChannels The total amount of channels on the server or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError GetChannelsAmount(int &iChannels);

    /*!
     * @brief Request the list of all channels from the backend.
     * @param channels The channel group to add the channels to.
     * @param bRadio True to get the radio channels, false to get the TV channels.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVRClientError GetChannels(CPVRChannelGroup &channels, bool bRadio);

    //@}
    /** @name PVR recording methods */
    //@{

    /*!
     * @brief Get the total amount of recordings from the backend.
     * @param deleted True to return deleted recordings.
     * @param iRecordings The total amount of recordings on the server or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError GetRecordingsAmount(bool deleted, int &iRecordings);

    /*!
     * @brief Request the list of all recordings from the backend.
     * @param results The container to add the recordings to.
     * @param deleted True to return deleted recordings.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVRClientError GetRecordings(CPVRRecordings *results, bool deleted);

    /*!
     * @brief Delete a recording on the backend.
     * @param recording The recording to delete.
     * @return PVR_ERROR_NO_ERROR if the recording has been deleted successfully.
     */
    PVRClientError DeleteRecording(const CPVRRecording &recording);

    /*!
     * @brief Undelete a recording on the backend.
     * @param recording The recording to undelete.
     * @return PVR_ERROR_NO_ERROR if the recording has been undeleted successfully.
     */
    PVRClientError UndeleteRecording(const CPVRRecording &recording);

    /*!
     * @brief Delete all recordings permanent which in the deleted folder on the backend.
     * @return PVR_ERROR_NO_ERROR if the recordings has been deleted successfully.
     */
    PVRClientError DeleteAllRecordingsFromTrash();

    /*!
     * @brief Rename a recording on the backend.
     * @param recording The recording to rename.
     * @return PVR_ERROR_NO_ERROR if the recording has been renamed successfully.
     */
    PVRClientError RenameRecording(const CPVRRecording &recording);

    /*!
     * @brief Set the lifetime of a recording on the backend.
     * @param recording The recording to set the lifetime for. recording.m_iLifetime contains the new lifetime value.
     * @return PVR_ERROR_NO_ERROR if the recording's lifetime has been set successfully.
     */
    PVRClientError SetRecordingLifetime(const CPVRRecording &recording);

    /*!
     * @brief Set the play count of a recording on the backend.
     * @param recording The recording to set the play count.
     * @param count Play count.
     * @return PVR_ERROR_NO_ERROR if the recording's play count has been set successfully.
     */
    PVRClientError SetRecordingPlayCount(const CPVRRecording &recording, int count);

    /*!
    * @brief Set the last watched position of a recording on the backend.
    * @param recording The recording.
    * @param lastplayedposition The last watched position in seconds
    * @return PVR_ERROR_NO_ERROR if the position has been stored successfully.
    */
    PVRClientError SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition);

    /*!
    * @brief Retrieve the last watched position of a recording on the backend.
    * @param recording The recording.
    * @param iPosition The last watched position in seconds or -1 on error
    * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
    */
    PVRClientError GetRecordingLastPlayedPosition(const CPVRRecording &recording, int &iPosition);

    /*!
    * @brief Retrieve the edit decision list (EDL) from the backend.
    * @param recording The recording.
    * @param edls The edit decision list (empty on error).
    * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
    */
    PVRClientError GetRecordingEdl(const CPVRRecording &recording, std::vector<PVR_EDL_ENTRY> &edls);

    /*!
    * @brief Retrieve the edit decision list (EDL) from the backend.
    * @param epgTag The EPG tag.
    * @param edls The edit decision list (empty on error).
    * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
    */
    PVRClientError GetEpgTagEdl(const std::shared_ptr<const CPVREpgInfoTag> &epgTag, std::vector<PVR_EDL_ENTRY> &edls);

    //@}
    /** @name PVR timer methods */
    //@{

    /*!
     * @brief Get the total amount of timers from the backend.
     * @param iTimers The total amount of timers on the backend or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError GetTimersAmount(int &iTimers);

    /*!
     * @brief Request the list of all timers from the backend.
     * @param results The container to store the result in.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVRClientError GetTimers(CPVRTimersContainer *results);

    /*!
     * @brief Add a timer on the backend.
     * @param timer The timer to add.
     * @return PVR_ERROR_NO_ERROR if the timer has been added successfully.
     */
    PVRClientError AddTimer(const CPVRTimerInfoTag &timer);

    /*!
     * @brief Delete a timer on the backend.
     * @param timer The timer to delete.
     * @param bForce Set to true to delete a timer that is currently recording a program.
     * @return PVR_ERROR_NO_ERROR if the timer has been deleted successfully.
     */
    PVRClientError DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce = false);

    /*!
     * @brief Update the timer information on the server.
     * @param timer The timer to update.
     * @return PVR_ERROR_NO_ERROR if the timer has been updated successfully.
     */
    PVRClientError UpdateTimer(const CPVRTimerInfoTag &timer);

    /*!
     * @brief Get all timer types supported by the backend.
     * @param results The container to store the result in.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVRClientError GetTimerTypes(std::vector<std::shared_ptr<CPVRTimerType>>& results) const;

    //@}
    /** @name PVR live stream methods */
    //@{

    /*!
     * @brief Open a live stream on the server.
     * @param channel The channel to stream.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError OpenLiveStream(const std::shared_ptr<CPVRChannel> &channel);

    /*!
     * @brief Close an open live stream.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError CloseLiveStream();

    /*!
     * @brief Read from an open live stream.
     * @param lpBuf The buffer to store the data in.
     * @param uiBufSize The amount of bytes to read.
     * @param iRead The amount of bytes that were actually read from the stream.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError ReadLiveStream(void* lpBuf, int64_t uiBufSize, int &iRead);

    /*!
     * @brief Seek in a live stream on a backend.
     * @param iFilePosition The position to seek to.
     * @param iWhence ?
     * @param iPosition The new position or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError SeekLiveStream(int64_t iFilePosition, int iWhence, int64_t &iPosition);

    /*!
     * @brief Get the lenght of the currently playing live stream, if any.
     * @param iLength The total length of the stream that's currently being read or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError GetLiveStreamLength(int64_t &iLength);

    /*!
     * @brief (Un)Pause a stream.
     * @param bPaused True to pause the stream, false to unpause.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError PauseStream(bool bPaused);

    /*!
     * @brief Get the signal quality of the stream that's currently open.
     * @param qualityinfo The signal quality.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError SignalQuality(PVR_SIGNAL_STATUS &qualityinfo);

    /*!
     * @brief Get the descramble information of the stream that's currently open.
     * @param descrambleinfo The descramble information.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError GetDescrambleInfo(PVR_DESCRAMBLE_INFO &descrambleinfo) const;

    /*!
     * @brief Fill the file item for a channel with the properties required for playback. Values are obtained from the PVR backend.
     * @param fileItem The file item to be filled.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError FillChannelStreamFileItem(CFileItem &fileItem);

    /*!
     * @brief Check whether PVR backend supports pausing the currently playing stream
     * @param bCanPause True if the stream can be paused, false otherwise.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError CanPauseStream(bool &bCanPause) const;

    /*!
     * @brief Check whether PVR backend supports seeking for the currently playing stream
     * @param bCanSeek True if the stream can be seeked, false otherwise.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError CanSeekStream(bool &bCanSeek) const;

    /*!
     * @brief Notify the pvr addon/demuxer that Kodi wishes to seek the stream by time
     * @param time The absolute time since stream start
     * @param backwards True to seek to keyframe BEFORE time, else AFTER
     * @param startpts can be updated to point to where display should start
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     * @remarks Optional, and only used if addon has its own demuxer.
     */
    PVRClientError SeekTime(double time, bool backwards, double *startpts);

    /*!
     * @brief Notify the pvr addon/demuxer that Kodi wishes to change playback speed
     * @param speed The requested playback speed
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     * @remarks Optional, and only used if addon has its own demuxer.
     */
    PVRClientError SetSpeed(int speed);

    //@}
    /** @name PVR recording stream methods */
    //@{

    /*!
     * @brief Open a recording on the server.
     * @param recording The recording to open.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError OpenRecordedStream(const std::shared_ptr<CPVRRecording> &recording);

    /*!
     * @brief Close an open recording stream.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError CloseRecordedStream();

    /*!
     * @brief Read from an open recording stream.
     * @param lpBuf The buffer to store the data in.
     * @param uiBufSize The amount of bytes to read.
     * @param iRead The amount of bytes that were actually read from the stream.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError ReadRecordedStream(void* lpBuf, int64_t uiBufSize, int &iRead);

    /*!
     * @brief Seek in a recording stream on a backend.
     * @param iFilePosition The position to seek to.
     * @param iWhence ?
     * @param iPosition The new position or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError SeekRecordedStream(int64_t iFilePosition, int iWhence, int64_t &iPosition);

    /*!
     * @brief Get the lenght of the currently playing recording stream, if any.
     * @param iLength The total length of the stream that's currently being read or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError GetRecordedStreamLength(int64_t &iLength);

    /*!
     * @brief Fill the file item for a recording with the properties required for playback. Values are obtained from the PVR backend.
     * @param fileItem The file item to be filled.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError FillRecordingStreamFileItem(CFileItem &fileItem);

    //@}
    /** @name PVR demultiplexer methods */
    //@{

    /*!
     * @brief Reset the demultiplexer in the add-on.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError DemuxReset();

    /*!
     * @brief Abort the demultiplexer thread in the add-on.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError DemuxAbort();

    /*!
     * @brief Flush all data that's currently in the demultiplexer buffer in the add-on.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError DemuxFlush();

    /*!
     * @brief Read a packet from the demultiplexer.
     * @param packet The packet read.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError DemuxRead(DemuxPacket* &packet);

    static const char *ToString(const PVRClientError error);

    /*!
     * @brief Check whether the currently playing stream, if any, is a real-time stream.
     * @param bRealTime True if real-time, false otherwise.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError IsRealTimeStream(bool &bRealTime) const;

    /*!
     * @brief Get Stream times for the currently playing stream, if any (will be moved to inputstream).
     * @param times The stream times.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError GetStreamTimes(PVR_STREAM_TIMES *times);

    /*!
     * @brief reads the client's properties.
     * @return True on success, false otherwise.
     */
    bool GetAddonProperties(void);

    /*!
     * @brief Get the client's menu hooks.
     * @return The hooks. Guaranteed never to be null.
     */
    std::shared_ptr<CPVRClientMenuHooks> GetMenuHooks();

    /*!
     * @brief Call one of the menu hooks of the client.
     * @param hook The hook to call.
     * @param item The item associated with the hook to be called.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError CallMenuHook(const CPVRClientMenuHook &hook, const CFileItemPtr &item);

    /*!
     * @brief Propagate power management events to this add-on
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError OnSystemSleep();
    PVRClientError OnSystemWake();
    PVRClientError OnPowerSavingActivated();
    PVRClientError OnPowerSavingDeactivated();

    /*!
     * @brief Get the priority of this client. Larger value means higher priority.
     * @return The priority.
     */
    int GetPriority() const;

    /*!
     * @brief Set a new priority for this client.
     * @param iPriority The new priority.
     */
    void SetPriority(int iPriority);

    /*!
     * @brief Obtain the chunk size to use when reading streams.
     * @param iChunkSize the chunk size in bytes.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVRClientError GetStreamReadChunkSize(int &iChunkSize);

    /*!
     * @brief Get the interface table used between addon and Kodi.
     * @todo This function will be removed after old callback library system is removed.
     */
    AddonInstance_PVR* GetInstanceInterface() { return m_struct.get(); }

  private:
    /*!
     * @brief Resets all class members to their defaults. Called by the constructors.
     */
    void ResetProperties(int iClientId = PVR_INVALID_CLIENT_ID);

    /*!
     * @brief Copy over group info from xbmcGroup to addonGroup.
     * @param xbmcGroup The group on XBMC's side.
     * @param addonGroup The group on the addon's side.
     */
    static void WriteClientGroupInfo(const CPVRChannelGroup &xbmcGroup, PVR_CHANNEL_GROUP &addonGroup);

    /*!
     * @brief Copy over recording info from xbmcRecording to addonRecording.
     * @param xbmcRecording The recording on XBMC's side.
     * @param addonRecording The recording on the addon's side.
     */
    static void WriteClientRecordingInfo(const CPVRRecording &xbmcRecording, PVR_RECORDING &addonRecording);

    /*!
     * @brief Copy over timer info from xbmcTimer to addonTimer.
     * @param xbmcTimer The timer on XBMC's side.
     * @param addonTimer The timer on the addon's side.
     */
    static void WriteClientTimerInfo(const CPVRTimerInfoTag &xbmcTimer, PVR_TIMER &addonTimer);

    /*!
     * @brief Copy over channel info from xbmcChannel to addonClient.
     * @param xbmcChannel The channel on XBMC's side.
     * @param addonChannel The channel on the addon's side.
     */
    static void WriteClientChannelInfo(const std::shared_ptr<CPVRChannel> &xbmcChannel, PVR_CHANNEL &addonChannel);

    /*!
     * @brief Write the given addon properties to the properties of the given file item.
     * @param properties Pointer to an array of addon properties.
     * @param iPropertyCount The number of properties contained in the addon properties array.
     * @param fileItem The item the addon properties shall be written to.
     */
    static void WriteFileItemProperties(const PVR_NAMED_VALUE *properties, unsigned int iPropertyCount, CFileItem &fileItem);

    /*!
     * @brief Whether a channel can be played by this add-on
     * @param channel The channel to check.
     * @return True when it can be played, false otherwise.
     */
    bool CanPlayChannel(const std::shared_ptr<CPVRChannel> &channel) const;

    /*!
     * @brief Stop this instance, if it is currently running.
     */
    void StopRunningInstance();

    /*!
     * @brief Wraps an addon function call in order to do common pre and post function invocation actions.
     * @param strFunctionName The function name, for logging purposes.
     * @param function The function to wrap. It has to have return type PVRClientError and must take one parameter of type const AddonInstance*.
     * @param bIsImplemented If false, this method will return PVR_ERROR_NOT_IMPLEMENTED.
     * @param bCheckReadyToUse If true, this method will check whether this instance is ready for use and return PVR_ERROR_SERVER_ERROR if it is not.
     * @return PVR_ERROR_NO_ERROR on success, any other PVR_ERROR_* value otherwise.
     */
    typedef KodiToAddonFuncTable_PVR AddonInstance;
    PVRClientError DoAddonCall(const char* strFunctionName,
                          std::function<PVRClientError(const AddonInstance*)> function,
                          bool bIsImplemented = true,
                          bool bCheckReadyToUse = true) const;

    std::atomic<bool>      m_bReadyToUse;          /*!< true if this add-on is initialised (ADDON_Create returned true), false otherwise */
    std::atomic<bool>      m_bBlockAddonCalls;     /*!< true if no add-on API calls are allowed */
    PVRClientConnectionState m_connectionState;    /*!< the backend connection state */
    PVRClientConnectionState m_prevConnectionState;  /*!< the previous backend connection state */
    bool                   m_ignoreClient;         /*!< signals to PVRManager to ignore this client until it has been connected */
    std::vector<std::shared_ptr<CPVRTimerType>> m_timertypes; /*!< timer types supported by this backend */
    int                    m_iClientId;            /*!< unique ID of the client */
    mutable int            m_iPriority;            /*!< priority of the client */
    mutable bool           m_bPriorityFetched;

    /* cached data */
    std::string            m_strBackendName;       /*!< the cached backend version */
    std::string            m_strBackendVersion;    /*!< the cached backend version */
    std::string            m_strConnectionString;  /*!< the cached connection string */
    std::string            m_strFriendlyName;      /*!< the cached friendly name */
    std::string            m_strBackendHostname;   /*!< the cached backend hostname */
    std::shared_ptr<CPVRClientCapabilities> m_clientCapabilities; /*!< the cached add-on's capabilities */
    std::shared_ptr<CPVRClientMenuHooks> m_menuhooks; /*!< the menu hooks for this add-on */

    /* stored strings to make sure const char* members in PVR_PROPERTIES stay valid */
    std::string            m_strUserPath;         /*!< @brief translated path to the user profile */
    std::string            m_strClientPath;       /*!< @brief translated path to this add-on */

    mutable CCriticalSection m_critSection;

    std::unique_ptr<AddonInstance_PVR> m_struct;
  };
}
