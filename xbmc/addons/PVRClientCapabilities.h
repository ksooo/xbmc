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

#include <memory>
#include <string>
#include <utility>
#include <vector>

struct PVR_ADDON_CAPABILITIES;

namespace PVR
{
  class CPVRClientCapabilities
  {
  public:
    CPVRClientCapabilities() = delete;
    virtual ~CPVRClientCapabilities() = default;

    explicit CPVRClientCapabilities(const PVR_ADDON_CAPABILITIES& addonCapabilities);

    /////////////////////////////////////////////////////////////////////////////////
    //
    // Channels
    //
    /////////////////////////////////////////////////////////////////////////////////

    /*!
     * @brief Check whether this add-on supports TV channels.
     * @return True if recordings are supported, false otherwise.
     */
    bool SupportsTV() const;

    /*!
     * @brief Check whether this add-on supports radio channels.
     * @return True if recordings are supported, false otherwise.
     */
    bool SupportsRadio() const;

    /*!
     * @brief Check whether this add-on supports channel groups.
     * @return True if recordings are supported, false otherwise.
     */
    bool SupportsChannelGroups() const;

    /*!
     * @brief Check whether this add-on supports scanning for new channels on the backend.
     * @return True if recordings are supported, false otherwise.
     */
    bool SupportsChannelScan() const;

    /*!
     * @brief Check whether this add-on supports the following functions: DeleteChannel, RenameChannel, MoveChannel, DialogChannelSettings and DialogAddChannel.
     * @return True if recordings are supported, false otherwise.
     */
    bool SupportsChannelSettings() const;

    /*!
     * @brief Check whether this add-on supports descramble information for playing channels.
     * @return True if recordings are supported, false otherwise.
     */
    bool SupportsDescrambleInfo() const;

    /////////////////////////////////////////////////////////////////////////////////
    //
    // EPG
    //
    /////////////////////////////////////////////////////////////////////////////////

    /*!
     * @brief Check whether this add-on provides EPG information.
     * @return True if recordings are supported, false otherwise.
     */
    bool SupportsEPG() const;

    /////////////////////////////////////////////////////////////////////////////////
    //
    // Timers
    //
    /////////////////////////////////////////////////////////////////////////////////

    /*!
     * @brief Check whether this add-on supports the creation and editing of timers.
     * @return True if recordings are supported, false otherwise.
     */
    bool SupportsTimers() const;

    /////////////////////////////////////////////////////////////////////////////////
    //
    // Recordings
    //
    /////////////////////////////////////////////////////////////////////////////////

    /*!
     * @brief Check whether this add-on supports recordings.
     * @return True if recordings are supported, false otherwise.
     */
    bool SupportsRecordings() const;

    /*!
     * @brief Check whether this add-on supports undelete of deleted recordings.
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordingsUndelete() const;

    /*!
     * @brief Check whether this add-on supports play count for recordings.
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordingsPlayCount() const;

    /*!
     * @brief Check whether this add-on supports store/retrieve of last played position for recordings..
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordingsLastPlayedPosition() const;

    /*!
     * @brief Check whether this add-on supports retrieving an edit decision list for recordings.
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordingsEdl() const;

    /*!
     * @brief Check whether this add-on supports renaming recordings..
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordingsRename() const;

    /*!
     * @brief Check whether this add-on supports changing lifetime of recording.
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordingsLifetimeChange() const;

    /*!
     * @brief Obtain a list with all possible values for recordings lifetime.
     * @param list out, the list with the values or an empty list, if lifetime is not supported.
     */
    void GetRecordingsLifetimeValues(std::vector<std::pair<std::string, int>> &list) const;

    /////////////////////////////////////////////////////////////////////////////////
    //
    // Streams
    //
    /////////////////////////////////////////////////////////////////////////////////

    /*!
     * @brief Check whether this add-on provides an input stream. false if Kodi handles the stream.
     * @return True if supported, false otherwise.
     */
    bool HandlesInputStream() const;

    /*!
     * @brief Check whether this add-on demultiplexes packets.
     * @return True if supported, false otherwise.
     */
    bool HandlesDemuxing() const;

  private:
    void InitRecordingsLifetimeValues();

    std::unique_ptr<PVR_ADDON_CAPABILITIES> m_addonCapabilities;
    std::vector<std::pair<std::string, int>> m_recordingsLifetimeValues;
  };

  typedef std::shared_ptr<CPVRClientCapabilities> CPVRClientCapabilitiesPtr;
}
