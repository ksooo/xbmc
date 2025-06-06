/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRRecording.h"

#include "ServiceBroker.h"
#include "cores/EdlEdit.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/providers/PVRProvider.h"
#include "pvr/providers/PVRProviders.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace PVR;

using namespace std::chrono_literals;

const std::string CPVRRecording::IMAGE_OWNER_PATTERN = "pvrrecording";

CPVRRecording::CPVRRecording()
  : m_iconPath(IMAGE_OWNER_PATTERN),
    m_thumbnailPath(IMAGE_OWNER_PATTERN),
    m_fanartPath(IMAGE_OWNER_PATTERN),
    m_parentalRatingIcon(IMAGE_OWNER_PATTERN)
{
  Reset();
}

CPVRRecording::CPVRRecording(const PVR_RECORDING& recording, unsigned int iClientId)
  : m_iconPath(recording.strIconPath ? recording.strIconPath : "", IMAGE_OWNER_PATTERN),
    m_thumbnailPath(recording.strThumbnailPath ? recording.strThumbnailPath : "",
                    IMAGE_OWNER_PATTERN),
    m_fanartPath(recording.strFanartPath ? recording.strFanartPath : "", IMAGE_OWNER_PATTERN),
    m_parentalRatingIcon(recording.strParentalRatingIcon ? recording.strParentalRatingIcon : "",
                         IMAGE_OWNER_PATTERN)
{
  Reset();

  if (recording.strRecordingId)
    m_strRecordingId = recording.strRecordingId;
  if (recording.strTitle)
    m_strTitle = recording.strTitle;
  if (recording.strTitleExtraInfo)
    m_titleExtraInfo = recording.strTitleExtraInfo;
  if (recording.strEpisodeName)
    m_strShowTitle = recording.strEpisodeName;
  m_iSeason = recording.iSeriesNumber;
  m_iEpisode = recording.iEpisodeNumber;
  m_episodePartNumber = recording.iEpisodePartNumber;
  if (recording.iYear > 0)
    SetYear(recording.iYear);
  m_iClientId = iClientId;
  m_recordingTime =
      recording.recordingTime +
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iPVRTimeCorrection;
  m_iPriority = recording.iPriority;
  m_iLifetime = recording.iLifetime;
  // Deleted recording is placed at the root of the deleted view
  if (recording.strDirectory && !recording.bIsDeleted)
    m_strDirectory = recording.strDirectory;
  if (recording.strPlot)
    m_strPlot = recording.strPlot;
  if (recording.strPlotOutline)
    m_strPlotOutline = recording.strPlotOutline;
  if (recording.strChannelName)
    m_strChannelName = recording.strChannelName;
  m_bIsDeleted = recording.bIsDeleted;
  m_iEpgEventId = recording.iEpgEventId;
  m_iChannelUid = recording.iChannelUid;
  if (recording.strFirstAired && strlen(recording.strFirstAired) > 0)
    m_firstAired.SetFromW3CDateTime(recording.strFirstAired);
  m_iFlags = recording.iFlags;
  if (recording.sizeInBytes >= 0)
    m_sizeInBytes = recording.sizeInBytes;
  if (recording.strProviderName)
    m_strProviderName = recording.strProviderName;
  m_iClientProviderUid = recording.iClientProviderUid;

  SetGenre(recording.iGenreType, recording.iGenreSubType,
           recording.strGenreDescription ? recording.strGenreDescription : "");
  CVideoInfoTag::SetPlayCount(recording.iPlayCount);
  if (recording.iLastPlayedPosition > 0 && recording.iDuration > recording.iLastPlayedPosition)
    CVideoInfoTag::SetResumePoint(recording.iLastPlayedPosition, recording.iDuration, "");
  SetDuration(recording.iDuration);

  m_parentalRating = recording.iParentalRating;
  if (recording.strParentalRatingCode)
    m_parentalRatingCode = recording.strParentalRatingCode;
  if (recording.strParentalRatingSource)
    m_parentalRatingSource = recording.strParentalRatingSource;

  //  As the channel a recording was done on (probably long time ago) might no longer be
  //  available today prefer addon-supplied channel type (tv/radio) over channel attribute.
  if (recording.channelType != PVR_RECORDING_CHANNEL_TYPE_UNKNOWN)
  {
    m_bRadio = recording.channelType == PVR_RECORDING_CHANNEL_TYPE_RADIO;
  }
  else
  {
    const std::shared_ptr<const CPVRChannel> channel(Channel());
    if (channel)
    {
      m_bRadio = channel->IsRadio();
    }
    else
    {
      const std::shared_ptr<const CPVRClient> client =
          CServiceBroker::GetPVRManager().GetClient(m_iClientId);
      bool bSupportsRadio = client && client->GetClientCapabilities().SupportsRadio();
      if (bSupportsRadio && client && client->GetClientCapabilities().SupportsTV())
      {
        CLog::Log(LOGWARNING, "Unable to determine channel type. Defaulting to TV.");
        m_bRadio = false; // Assume TV.
      }
      else
      {
        m_bRadio = bSupportsRadio;
      }
    }
  }

  UpdatePath();
}

bool CPVRRecording::operator==(const CPVRRecording& right) const
{
  std::unique_lock lock(m_critSection);
  return (this == &right) ||
         (m_strRecordingId == right.m_strRecordingId && m_iClientId == right.m_iClientId &&
          m_strChannelName == right.m_strChannelName && m_recordingTime == right.m_recordingTime &&
          GetDuration() == right.GetDuration() && m_strPlotOutline == right.m_strPlotOutline &&
          m_strPlot == right.m_strPlot && m_iPriority == right.m_iPriority &&
          m_iLifetime == right.m_iLifetime && m_strDirectory == right.m_strDirectory &&
          m_strFileNameAndPath == right.m_strFileNameAndPath && m_strTitle == right.m_strTitle &&
          m_strShowTitle == right.m_strShowTitle && m_iSeason == right.m_iSeason &&
          m_iEpisode == right.m_iEpisode && GetPremiered() == right.GetPremiered() &&
          m_iconPath == right.m_iconPath && m_thumbnailPath == right.m_thumbnailPath &&
          m_fanartPath == right.m_fanartPath && m_iRecordingId == right.m_iRecordingId &&
          m_bIsDeleted == right.m_bIsDeleted && m_iEpgEventId == right.m_iEpgEventId &&
          m_iChannelUid == right.m_iChannelUid && m_bRadio == right.m_bRadio &&
          m_genre == right.m_genre && m_iGenreType == right.m_iGenreType &&
          m_iGenreSubType == right.m_iGenreSubType && m_firstAired == right.m_firstAired &&
          m_iFlags == right.m_iFlags && m_sizeInBytes == right.m_sizeInBytes &&
          m_strProviderName == right.m_strProviderName &&
          m_iClientProviderUid == right.m_iClientProviderUid &&
          m_parentalRating == right.m_parentalRating &&
          m_parentalRatingCode == right.m_parentalRatingCode &&
          m_parentalRatingIcon == right.m_parentalRatingIcon &&
          m_parentalRatingSource == right.m_parentalRatingSource &&
          m_episodePartNumber == right.m_episodePartNumber &&
          m_titleExtraInfo == right.m_titleExtraInfo);
}

void CPVRRecording::Serialize(CVariant& value) const
{
  CVideoInfoTag::Serialize(value);

  value["channel"] = m_strChannelName;
  value["lifetime"] = m_iLifetime;
  value["directory"] = m_strDirectory;
  value["icon"] = ClientIconPath();
  value["starttime"] = m_recordingTime.IsValid() ? m_recordingTime.GetAsDBDateTime() : "";
  value["endtime"] = m_recordingTime.IsValid() ? EndTimeAsUTC().GetAsDBDateTime() : "";
  value["recordingid"] = m_iRecordingId;
  value["isdeleted"] = m_bIsDeleted;
  value["epgeventid"] = m_iEpgEventId;
  value["channeluid"] = m_iChannelUid;
  value["radio"] = m_bRadio;
  value["genre"] = m_genre;
  value["parentalrating"] = m_parentalRating;
  value["parentalratingcode"] = m_parentalRatingCode;
  value["parentalratingicon"] = ClientParentalRatingIconPath();
  value["parentalratingsource"] = m_parentalRatingSource;
  value["episodepart"] = m_episodePartNumber;
  value["titleextrainfo"] = m_titleExtraInfo;

  if (!value.isMember("art"))
    value["art"] = CVariant(CVariant::VariantTypeObject);
  if (!ClientThumbnailPath().empty())
    value["art"]["thumb"] = ClientThumbnailPath();
  if (!ClientFanartPath().empty())
    value["art"]["fanart"] = ClientFanartPath();

  value["clientid"] = m_iClientId;
}

void CPVRRecording::ToSortable(SortItem& sortable, Field field) const
{
  std::unique_lock lock(m_critSection);
  if (field == FieldSize)
    sortable[FieldSize] = m_sizeInBytes;
  else if (field == FieldProvider)
    sortable[FieldProvider] = StringUtils::Format("{} {}", m_iClientId, m_iClientProviderUid);
  else
    CVideoInfoTag::ToSortable(sortable, field);
}

void CPVRRecording::Reset()
{
  m_strRecordingId.clear();
  m_iClientId = PVR_CLIENT_INVALID_UID;
  m_strChannelName.clear();
  m_strDirectory.clear();
  m_iPriority = -1;
  m_iLifetime = -1;
  m_strFileNameAndPath.clear();
  m_bGotMetaData = false;
  m_iRecordingId = 0;
  m_bIsDeleted = false;
  m_bInProgress = true;
  m_iEpgEventId = EPG_TAG_INVALID_UID;
  m_iSeason = PVR_RECORDING_INVALID_SERIES_EPISODE;
  m_iEpisode = PVR_RECORDING_INVALID_SERIES_EPISODE;
  m_episodePartNumber = PVR_RECORDING_INVALID_SERIES_EPISODE;
  m_iChannelUid = PVR_CHANNEL_INVALID_UID;
  m_bRadio = false;
  m_iFlags = PVR_RECORDING_FLAG_UNDEFINED;
  {
    std::unique_lock lock(m_critSection);
    m_sizeInBytes = 0;
  }
  m_strProviderName.clear();
  m_iClientProviderUid = PVR_PROVIDER_INVALID_UID;

  m_recordingTime.Reset();

  m_parentalRating = 0;
  m_parentalRatingCode.clear();
  m_parentalRatingSource.clear();
  m_titleExtraInfo.clear();

  CVideoInfoTag::Reset();
}

bool CPVRRecording::Delete() const
{
  std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  return client && (client->DeleteRecording(*this) == PVR_ERROR_NO_ERROR);
}

bool CPVRRecording::Undelete() const
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  return client && (client->UndeleteRecording(*this) == PVR_ERROR_NO_ERROR);
}

bool CPVRRecording::Rename(std::string_view strNewName)
{
  m_strTitle = strNewName;
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  return client && (client->RenameRecording(*this) == PVR_ERROR_NO_ERROR);
}

bool CPVRRecording::SetPlayCount(int count)
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsPlayCount())
  {
    if (client->SetRecordingPlayCount(*this, count) != PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::SetPlayCount(count);
}

bool CPVRRecording::IncrementPlayCount()
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsPlayCount())
  {
    if (client->SetRecordingPlayCount(*this, CVideoInfoTag::GetPlayCount() + 1) !=
        PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::IncrementPlayCount();
}

bool CPVRRecording::SetResumePoint(const CBookmark& resumePoint)
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsLastPlayedPosition())
  {
    if (client->SetRecordingLastPlayedPosition(
            *this, MathUtils::round_int(resumePoint.timeInSeconds)) != PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::SetResumePoint(resumePoint);
}

bool CPVRRecording::SetResumePoint(double timeInSeconds,
                                   double totalTimeInSeconds,
                                   const std::string& playerState /* = "" */)
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsLastPlayedPosition())
  {
    if (client->SetRecordingLastPlayedPosition(*this, MathUtils::round_int(timeInSeconds)) !=
        PVR_ERROR_NO_ERROR)
      return false;
  }

  return CVideoInfoTag::SetResumePoint(timeInSeconds, totalTimeInSeconds, playerState);
}

CBookmark CPVRRecording::GetResumePoint() const
{
  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsLastPlayedPosition() &&
      m_resumePointRefetchTimeout.IsTimePast())
  {
    // @todo: root cause should be fixed. details: https://github.com/xbmc/xbmc/pull/14961
    m_resumePointRefetchTimeout.Set(10s); // update resume point from backend at most every 10 secs

    int pos = -1;
    client->GetRecordingLastPlayedPosition(*this, pos);

    if (pos >= 0)
    {
      CBookmark resumePoint(CVideoInfoTag::GetResumePoint());
      resumePoint.timeInSeconds = pos;
      resumePoint.totalTimeInSeconds = (pos == 0) ? 0 : m_duration;
      auto* pThis{const_cast<CPVRRecording*>(this)};
      pThis->CVideoInfoTag::SetResumePoint(resumePoint);
    }
  }
  return CVideoInfoTag::GetResumePoint();
}

bool CPVRRecording::UpdateRecordingSize()
{
  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsSize() &&
      m_recordingSizeRefetchTimeout.IsTimePast())
  {
    // @todo: root cause should be fixed. details: https://github.com/xbmc/xbmc/pull/14961
    m_recordingSizeRefetchTimeout.Set(10s); // update size from backend at most every 10 secs

    int64_t sizeInBytes = -1;
    client->GetRecordingSize(*this, sizeInBytes);

    std::unique_lock lock(m_critSection);
    if (sizeInBytes >= 0 && sizeInBytes != m_sizeInBytes)
    {
      m_sizeInBytes = sizeInBytes;
      return true;
    }
  }

  return false;
}

void CPVRRecording::UpdateMetadata(CVideoDatabase& db, const CPVRClient& client)
{
  if (m_bGotMetaData || !db.IsOpen())
    return;

  if (!client.GetClientCapabilities().SupportsRecordingsPlayCount())
    CVideoInfoTag::SetPlayCount(db.GetPlayCount(m_strFileNameAndPath));

  if (!client.GetClientCapabilities().SupportsRecordingsLastPlayedPosition())
  {
    CBookmark resumePoint;
    if (db.GetResumeBookMark(m_strFileNameAndPath, resumePoint))
      CVideoInfoTag::SetResumePoint(resumePoint);
  }

  m_lastPlayed = db.GetLastPlayed(m_strFileNameAndPath);
  db.GetStreamDetails(m_strFileNameAndPath, m_streamDetails);

  m_bGotMetaData = true;
}

void CPVRRecording::DeleteMetadata(CVideoDatabase& db) const
{
  db.BeginTransaction();
  if (db.EraseAllForFile(m_strFileNameAndPath))
    db.CommitTransaction();
  else
    db.RollbackTransaction();
}

std::vector<EDL::Edit> CPVRRecording::GetEdl() const
{
  std::vector<EDL::Edit> edls;

  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsRecordingsEdl())
    client->GetRecordingEdl(*this, edls);

  return edls;
}

void CPVRRecording::Update(const CPVRRecording& tag, const CPVRClient& client)
{
  m_strRecordingId = tag.m_strRecordingId;
  m_iClientId = tag.m_iClientId;
  m_strTitle = tag.m_strTitle;
  m_titleExtraInfo = tag.m_titleExtraInfo;
  m_strShowTitle = tag.m_strShowTitle;
  m_iSeason = tag.m_iSeason;
  m_iEpisode = tag.m_iEpisode;
  m_episodePartNumber = tag.m_episodePartNumber;
  SetPremiered(tag.GetPremiered());
  m_recordingTime = tag.m_recordingTime;
  m_iPriority = tag.m_iPriority;
  m_iLifetime = tag.m_iLifetime;
  m_strDirectory = tag.m_strDirectory;
  m_strPlot = tag.m_strPlot;
  m_strPlotOutline = tag.m_strPlotOutline;
  m_strChannelName = tag.m_strChannelName;
  m_genre = tag.m_genre;
  m_parentalRating = tag.m_parentalRating;
  m_parentalRatingCode = tag.m_parentalRatingCode;
  m_parentalRatingIcon = tag.m_parentalRatingIcon;
  m_parentalRatingSource = tag.m_parentalRatingSource;
  m_iconPath = tag.m_iconPath;
  m_thumbnailPath = tag.m_thumbnailPath;
  m_fanartPath = tag.m_fanartPath;
  m_bIsDeleted = tag.m_bIsDeleted;
  m_iEpgEventId = tag.m_iEpgEventId;
  m_iChannelUid = tag.m_iChannelUid;
  m_bRadio = tag.m_bRadio;
  m_firstAired = tag.m_firstAired;
  m_iFlags = tag.m_iFlags;
  {
    std::unique_lock lock(m_critSection);
    m_sizeInBytes = tag.m_sizeInBytes;
    m_strProviderName = tag.m_strProviderName;
    m_iClientProviderUid = tag.m_iClientProviderUid;
  }

  if (client.GetClientCapabilities().SupportsRecordingsPlayCount())
    CVideoInfoTag::SetPlayCount(tag.GetLocalPlayCount());

  if (client.GetClientCapabilities().SupportsRecordingsLastPlayedPosition())
    CVideoInfoTag::SetResumePoint(tag.GetLocalResumePoint());

  SetDuration(tag.GetDuration());

  if (m_iGenreType == EPG_GENRE_USE_STRING || m_iGenreSubType == EPG_GENRE_USE_STRING)
  {
    /* No type/subtype. Use the provided description */
    m_genre = tag.m_genre;
  }
  else
  {
    /* Determine genre description by type/subtype */
    m_genre = StringUtils::Split(
        CPVREpg::ConvertGenreIdToString(tag.m_iGenreType, tag.m_iGenreSubType),
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  }

  //Old Method of identifying TV show title and subtitle using m_strDirectory and strPlotOutline (deprecated)
  std::string strShow = StringUtils::Format("{} - ", g_localizeStrings.Get(20364));
  if (StringUtils::StartsWithNoCase(m_strPlotOutline, strShow))
  {
    CLog::Log(LOGWARNING, "PVR addon provides episode name in strPlotOutline which is deprecated");
    std::string strEpisode = m_strPlotOutline;
    std::string strTitle = m_strDirectory;

    size_t pos = strTitle.rfind('/');
    strTitle.erase(0, pos + 1);
    strEpisode.erase(0, strShow.size());
    m_strTitle = strTitle;
    pos = strEpisode.find('-');
    strEpisode.erase(0, pos + 2);
    m_strShowTitle = strEpisode;
  }

  UpdatePath();
}

void CPVRRecording::UpdatePath()
{
  m_strFileNameAndPath = CPVRRecordingsPath(m_bIsDeleted, m_bRadio, m_strDirectory, m_strTitle,
                                            m_iSeason, m_iEpisode, GetYear(), m_strShowTitle,
                                            m_strChannelName, m_recordingTime, m_strRecordingId)
                             .AsString();
}

const CDateTime& CPVRRecording::RecordingTimeAsLocalTime() const
{
  static CDateTime tmp;
  tmp.SetFromUTCDateTime(m_recordingTime);

  return tmp;
}

CDateTime CPVRRecording::EndTimeAsUTC() const
{
  unsigned int duration = GetDuration();
  return m_recordingTime + CDateTimeSpan(0, 0, duration / 60, duration % 60);
}

CDateTime CPVRRecording::EndTimeAsLocalTime() const
{
  CDateTime ret;
  ret.SetFromUTCDateTime(EndTimeAsUTC());
  return ret;
}

bool CPVRRecording::WillBeExpiredWithNewLifetime(int iLifetime) const
{
  if (iLifetime > 0)
    return (EndTimeAsUTC() + CDateTimeSpan(iLifetime, 0, 0, 0)) <= CDateTime::GetUTCDateTime();

  return false;
}

CDateTime CPVRRecording::ExpirationTimeAsLocalTime() const
{
  CDateTime ret;
  if (m_iLifetime > 0)
    ret = EndTimeAsLocalTime() + CDateTimeSpan(m_iLifetime, 0, 0, 0);

  return ret;
}

std::string CPVRRecording::GetTitleFromURL(const std::string& url)
{
  return CPVRRecordingsPath(url).GetTitle();
}

std::shared_ptr<CPVRChannel> CPVRRecording::Channel() const
{
  if (m_iChannelUid != PVR_CHANNEL_INVALID_UID)
    return CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(m_iChannelUid,
                                                                          m_iClientId);

  return std::shared_ptr<CPVRChannel>();
}

int CPVRRecording::ChannelUid() const
{
  return m_iChannelUid;
}

int CPVRRecording::ClientID() const
{
  return m_iClientId;
}

std::shared_ptr<CPVRTimerInfoTag> CPVRRecording::GetRecordingTimer() const
{
  const std::vector<std::shared_ptr<CPVRTimerInfoTag>> recordingTimers =
      CServiceBroker::GetPVRManager().Timers()->GetActiveRecordings();

  for (const auto& timer : recordingTimers)
  {
    if (timer->ClientID() == ClientID() && timer->ClientChannelUID() == ChannelUid())
    {
      // first, match epg event uids, if available
      if (timer->UniqueBroadcastID() == BroadcastUid() &&
          timer->UniqueBroadcastID() != EPG_TAG_INVALID_UID)
        return timer;

      // alternatively, match start and end times
      const CDateTime timerStart =
          timer->StartAsUTC() - CDateTimeSpan(0, 0, timer->MarginStart(), 0);
      const CDateTime timerEnd = timer->EndAsUTC() + CDateTimeSpan(0, 0, timer->MarginEnd(), 0);
      if (timerStart <= RecordingTimeAsUTC() && timerEnd >= EndTimeAsUTC())
        return timer;
    }
  }
  return {};
}

bool CPVRRecording::IsInProgress() const
{
  // Note: It is not enough to only check recording time and duration against 'now'.
  //       Only the state of the related timer is a safe indicator that the backend
  //       actually is recording this.
  // Once the recording is known to not be in progress that will never change.
  if (m_bInProgress)
    m_bInProgress = GetRecordingTimer() != nullptr;
  return m_bInProgress;
}

void CPVRRecording::SetGenre(int iGenreType, int iGenreSubType, const std::string& strGenre)
{
  m_iGenreType = iGenreType;
  m_iGenreSubType = iGenreSubType;

  if ((iGenreType == EPG_GENRE_USE_STRING || iGenreSubType == EPG_GENRE_USE_STRING) &&
      !strGenre.empty())
  {
    /* Type and sub type are not given. Use the provided genre description if available. */
    m_genre = StringUtils::Split(
        strGenre,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  }
  else
  {
    /* Determine the genre description from the type and subtype IDs */
    m_genre = StringUtils::Split(
        CPVREpg::ConvertGenreIdToString(iGenreType, iGenreSubType),
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  }
}

std::string CPVRRecording::GetGenresLabel() const
{
  return StringUtils::Join(
      m_genre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
}

CDateTime CPVRRecording::FirstAired() const
{
  return m_firstAired;
}

void CPVRRecording::SetYear(int year)
{
  if (year > 0)
    m_premiered = CDateTime(year, 1, 1, 0, 0, 0);
}

int CPVRRecording::GetYear() const
{
  return m_premiered.IsValid() ? m_premiered.GetYear() : 0;
}

bool CPVRRecording::HasYear() const
{
  return m_premiered.IsValid();
}

bool CPVRRecording::IsNew() const
{
  return (m_iFlags & PVR_RECORDING_FLAG_IS_NEW) > 0;
}

bool CPVRRecording::IsPremiere() const
{
  return (m_iFlags & PVR_RECORDING_FLAG_IS_PREMIERE) > 0;
}

bool CPVRRecording::IsLive() const
{
  return (m_iFlags & PVR_RECORDING_FLAG_IS_LIVE) > 0;
}

bool CPVRRecording::IsFinale() const
{
  return (m_iFlags & PVR_RECORDING_FLAG_IS_FINALE) > 0;
}

int64_t CPVRRecording::GetSizeInBytes() const
{
  std::unique_lock lock(m_critSection);
  return m_sizeInBytes;
}

int CPVRRecording::ClientProviderUid() const
{
  std::unique_lock lock(m_critSection);
  return m_iClientProviderUid;
}

std::string CPVRRecording::ProviderName() const
{
  std::unique_lock lock(m_critSection);
  return m_strProviderName;
}

std::shared_ptr<CPVRProvider> CPVRRecording::GetDefaultProvider() const
{
  return CServiceBroker::GetPVRManager().Providers()->GetByClient(m_iClientId,
                                                                  PVR_PROVIDER_INVALID_UID);
}

bool CPVRRecording::HasClientProvider() const
{
  std::unique_lock lock(m_critSection);
  return m_iClientProviderUid != PVR_PROVIDER_INVALID_UID;
}

std::shared_ptr<CPVRProvider> CPVRRecording::GetProvider() const
{
  auto provider =
      CServiceBroker::GetPVRManager().Providers()->GetByClient(m_iClientId, m_iClientProviderUid);

  if (!provider)
    provider = GetDefaultProvider();

  return provider;
}

unsigned int CPVRRecording::GetParentalRating() const
{
  std::unique_lock lock(m_critSection);
  return m_parentalRating;
}

const std::string& CPVRRecording::GetParentalRatingCode() const
{
  std::unique_lock lock(m_critSection);
  return m_parentalRatingCode;
}

const std::string& CPVRRecording::GetParentalRatingIcon() const
{
  std::unique_lock lock(m_critSection);
  return m_parentalRatingIcon.GetLocalImage();
}

const std::string& CPVRRecording::GetParentalRatingSource() const
{
  std::unique_lock lock(m_critSection);
  return m_parentalRatingSource;
}

int CPVRRecording::EpisodePart() const
{
  std::unique_lock lock(m_critSection);
  return m_episodePartNumber;
}

const std::string& CPVRRecording::TitleExtraInfo() const
{
  std::unique_lock lock(m_critSection);
  return m_titleExtraInfo;
}
