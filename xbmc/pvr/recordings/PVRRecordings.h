/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <memory>
#include <string>

#include "threads/CriticalSection.h"

class CFileItem;
class CFileItemList;
class CVideoDatabase;

namespace PVR
{
  class CPVREpgInfoTag;
  class CPVRRecording;
  class CPVRRecordingUid;
  class CPVRRecordingsPath;

  class CPVRRecordings
  {
  public:
    CPVRRecordings();
    virtual ~CPVRRecordings(void);

    /**
     * @brief (re)load the recordings from the clients.
     * @return the number of recordings loaded.
     */
    int Load();

    /**
     * @brief unload all recordings.
     */
    void Unload();

    void UpdateFromClient(const std::shared_ptr<CPVRRecording> &tag);

    /**
     * @brief refresh the recordings list from the clients.
     */
    void Update(void);

    int GetNumTVRecordings() const;
    bool HasDeletedTVRecordings() const;
    int GetNumRadioRecordings() const;
    bool HasDeletedRadioRecordings() const;

    /**
     * @brief Deletes the item in question, be it a directory or a file
     * @param item the item to delete
     * @return whether the item was deleted successfully
     */
    bool Delete(const CFileItem &item);

    bool Undelete(const CFileItem &item);
    bool DeleteAllRecordingsFromTrash();
    bool RenameRecording(CFileItem &item, std::string &strNewName);
    bool SetRecordingsPlayCount(const std::shared_ptr<CFileItem> &item, int count);
    bool IncrementRecordingsPlayCount(const std::shared_ptr<CFileItem> &item);
    bool MarkWatched(const std::shared_ptr<CFileItem> &item, bool bWatched);

    /**
     * @brief Resets a recording's resume point, if any
     * @param item The item to process
     * @return True, if the item's resume point was reset successfully, false otherwise
     */
    bool ResetResumePoint(const std::shared_ptr<CFileItem> item);

    bool GetDirectory(const std::string& strPath, CFileItemList &items);
    std::shared_ptr<CFileItem> GetByPath(const std::string &path);
    std::shared_ptr<CPVRRecording> GetById(int iClientId, const std::string &strRecordingId) const;
    void GetAll(CFileItemList &items, bool bDeleted = false);
    std::shared_ptr<CFileItem> GetById(unsigned int iId) const;

    /*!
     * @brief Get the recording for the given epg tag, if any.
     * @param epgTag The epg tag.
     * @return The requested recording, or an empty recordingptr if none was found.
     */
    std::shared_ptr<CPVRRecording> GetRecordingForEpgTag(const std::shared_ptr<CPVREpgInfoTag> &epgTag) const;

  private:
    mutable CCriticalSection m_critSection;
    bool m_bIsUpdating = false;
    std::map<CPVRRecordingUid, std::shared_ptr<CPVRRecording>> m_recordings;
    unsigned int m_iLastId = 0;
    std::unique_ptr<CVideoDatabase> m_database;
    bool m_bDeletedTVRecordings = false;
    bool m_bDeletedRadioRecordings = false;
    unsigned int m_iTVRecordings = 0;
    unsigned int m_iRadioRecordings = 0;

    void UpdateFromClients(void);
    std::string TrimSlashes(const std::string &strOrig) const;
    bool IsDirectoryMember(const std::string &strDirectory, const std::string &strEntryDirectory, bool bGrouped) const;
    void GetSubDirectories(const CPVRRecordingsPath &recParentPath, CFileItemList *results);

    /**
     * @brief Get/Open the video database.
     * @return A reference to the video database.
     */
    CVideoDatabase& GetVideoDatabase();

    /**
     * @brief recursively deletes all recordings in the specified directory
     * @param item the directory
     * @return true if all recordings were deleted
     */
    bool DeleteDirectory(const CFileItem &item);
    bool DeleteRecording(const CFileItem &item);

    /**
     * @brief special value for parameter count of method ChangeRecordingsPlayCount
     */
    static const int INCREMENT_PLAY_COUNT = -1;

    /**
     * @brief change the playcount of the given recording or recursively of all children of the given recordings folder
     * @param item the recording or directory containing recordings
     * @param count the new playcount or INCREMENT_PLAY_COUNT to denote that the current playcount(s) are to be incremented by one
     * @return true if all playcounts were changed
     */
    bool ChangeRecordingsPlayCount(const std::shared_ptr<CFileItem> &item, int count);
  };
}
