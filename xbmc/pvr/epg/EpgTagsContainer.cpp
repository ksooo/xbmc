/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgTagsContainer.h"

#include "ServiceBroker.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/epg/EpgInfoTag.h"
#include "utils/log.h"

using namespace PVR;

namespace
{
const CDateTimeSpan ONE_SECOND(0, 0, 0, 1);
}

void CPVREpgTagsContainer::SetEpgID(int iEpgID)
{
  m_iEpgID = iEpgID;
  for (const auto& tag : m_changedTags)
    tag.second->SetEpgID(iEpgID);
}

void CPVREpgTagsContainer::SetChannelData(const std::shared_ptr<CPVREpgChannelData>& data)
{
  m_channelData = data;
  for (const auto& tag : m_changedTags)
    tag.second->SetChannelData(data);
}

bool CPVREpgTagsContainer::UpdateEntries(const CPVREpgTagsContainer& tags)
{
  for (const auto& tag : tags.m_changedTags)
    UpdateEntry(tag.second);

  return true;
}

void CPVREpgTagsContainer::FixOverlappingEvents(
    std::vector<std::shared_ptr<CPVREpgInfoTag>>& tags) const
{
  std::shared_ptr<CPVREpgInfoTag> previousTag;
  for (auto it = tags.begin(); it != tags.end();)
  {
    const std::shared_ptr<CPVREpgInfoTag> currentTag = *it;
    if (!previousTag)
    {
      previousTag = currentTag;
      ++it;
      continue;
    }

    const CDateTime currentStartTime = currentTag->StartAsUTC();
    if (previousTag->EndAsUTC() >= currentTag->EndAsUTC())
    {
      // delete the current tag. it's completely overlapped
      CLog::LogF(LOGWARNING,
                 "Erasing completely overlapped event from EPG timeline "
                 "(%u - %s - %s - %s) "
                 "(%u - %s - %s - %s).",
                 previousTag->UniqueBroadcastID(), previousTag->Title().c_str(),
                 previousTag->StartAsUTC().GetAsDBDateTime(),
                 previousTag->EndAsUTC().GetAsDBDateTime(), currentTag->UniqueBroadcastID(),
                 currentTag->Title().c_str(), currentTag->StartAsUTC().GetAsDBDateTime(),
                 currentTag->EndAsUTC().GetAsDBDateTime());
      it = tags.erase(it);

      if (m_previouslyEndedTag && m_previouslyEndedTag->StartAsUTC() == currentStartTime)
        m_previouslyEndedTag.reset();

      if (m_nowActiveTag && m_nowActiveTag->StartAsUTC() == currentStartTime)
        m_nowActiveTag.reset();

      if (m_nextUpcomingTag && m_nextUpcomingTag->StartAsUTC() == currentStartTime)
        m_nextUpcomingTag.reset();
    }
    else if (previousTag->EndAsUTC() > currentTag->StartAsUTC())
    {
      // fix the end time of the predecessor of the event
      CLog::LogF(LOGWARNING,
                 "Fixing partly overlapped event in EPG timeline "
                 "(%u - %s - %s - %s) "
                 "(%u - %s - %s - %s).",
                 previousTag->UniqueBroadcastID(), previousTag->Title().c_str(),
                 previousTag->StartAsUTC().GetAsDBDateTime(),
                 previousTag->EndAsUTC().GetAsDBDateTime(), currentTag->UniqueBroadcastID(),
                 currentTag->Title().c_str(), currentTag->StartAsUTC().GetAsDBDateTime(),
                 currentTag->EndAsUTC().GetAsDBDateTime());
      previousTag->SetEndFromUTC(currentTag->StartAsUTC() - ONE_SECOND);
      previousTag = currentTag;
      ++it;
    }
    else
    {
      previousTag = currentTag;
      ++it;
    }
  }
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::CreateEntry(
    const std::shared_ptr<CPVREpgInfoTag>& tag) const
{
  if (tag)
  {
    tag->SetChannelData(m_channelData);
    tag->SetEpgID(m_iEpgID);
  }
  return tag;
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgTagsContainer::CreateEntries(
    const std::vector<std::shared_ptr<CPVREpgInfoTag>>& tags) const
{
  for (auto& tag : tags)
  {
    tag->SetChannelData(m_channelData);
    tag->SetEpgID(m_iEpgID);
  }
  return tags;
}

bool CPVREpgTagsContainer::UpdateEntry(const std::shared_ptr<CPVREpgInfoTag>& tag)
{
  std::shared_ptr<CPVREpgInfoTag> infoTag = GetTag(tag->StartAsUTC());
  const bool bNewTag = infoTag == nullptr;
  if (bNewTag)
    infoTag.reset(new CPVREpgInfoTag());

  tag->SetChannelData(m_channelData);
  tag->SetEpgID(m_iEpgID);

  if (infoTag->Update(*tag, bNewTag))
    m_changedTags.insert({infoTag->StartAsUTC(), infoTag});

  return true;
}

bool CPVREpgTagsContainer::DeleteEntry(const std::shared_ptr<CPVREpgInfoTag>& tag)
{
  m_changedTags.erase(tag->StartAsUTC());
  m_deletedTags.insert({tag->StartAsUTC(), tag});

  if (m_previouslyEndedTag && m_previouslyEndedTag->StartAsUTC() == tag->StartAsUTC())
    m_previouslyEndedTag.reset();

  if (m_nowActiveTag && m_nowActiveTag->StartAsUTC() == tag->StartAsUTC())
    m_nowActiveTag.reset();

  if (m_nextUpcomingTag && m_nextUpcomingTag->StartAsUTC() == tag->StartAsUTC())
    m_nextUpcomingTag.reset();

  return true;
}

void CPVREpgTagsContainer::Cleanup(const CDateTime& time)
{
  for (auto it = m_changedTags.begin(); it != m_changedTags.end();)
  {
    if (it->second->EndAsUTC() < time)
    {
      if (m_previouslyEndedTag && m_previouslyEndedTag->StartAsUTC() == it->first)
        m_previouslyEndedTag.reset();

      if (m_nowActiveTag && m_nowActiveTag->StartAsUTC() == it->first)
        m_nowActiveTag.reset();

      if (m_nextUpcomingTag && m_nextUpcomingTag->StartAsUTC() == it->first)
        m_nextUpcomingTag.reset();

      const auto it1 = m_deletedTags.find(it->first);
      if (it1 != m_deletedTags.end())
        m_deletedTags.erase(it1);

      it = m_changedTags.erase(it);
    }
    else
    {
      ++it;
    }
  }

  if (m_database)
    m_database->DeleteEpgTags(m_iEpgID, time);
}

void CPVREpgTagsContainer::Clear()
{
  m_changedTags.clear();
}

bool CPVREpgTagsContainer::IsEmpty() const
{
  if (!m_changedTags.empty())
    return false;

  if (m_database)
    return !m_database->GetFirstStartTime(m_iEpgID).IsValid();

  return true;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetTag(const CDateTime& startTime) const
{
  const auto it = m_changedTags.find(startTime);
  if (it != m_changedTags.cend())
    return (*it).second;

  if (m_database)
    return CreateEntry(m_database->GetEpgTagByStartTime(m_iEpgID, startTime));

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetTag(unsigned int iUniqueBroadcastID) const
{
  if (iUniqueBroadcastID == EPG_TAG_INVALID_UID)
    return {};

  for (const auto& tag : m_changedTags)
  {
    if (tag.second->UniqueBroadcastID() == iUniqueBroadcastID)
      return tag.second;
  }

  if (m_database)
    return CreateEntry(m_database->GetEpgTagByUniqueBroadcastID(m_iEpgID, iUniqueBroadcastID));

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetTagBetween(const CDateTime& start,
                                                                    const CDateTime& end) const
{
  for (const auto& tag : m_changedTags)
  {
    if (tag.second->StartAsUTC() >= start)
    {
      if (tag.second->EndAsUTC() <= end)
        return tag.second;
      else
        break;
    }
  }

  if (m_database)
  {
    const std::vector<std::shared_ptr<CPVREpgInfoTag>> tags =
        CreateEntries(m_database->GetEpgTagsByMinStartMaxEndTime(m_iEpgID, start, end));
    if (!tags.empty())
    {
      if (tags.size() > 1)
        CLog::LogF(LOGWARNING, "Got multiple tags. Picking up the first.");

      return tags.front();
    }
  }

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetActiveTag(bool bUpdateIfNeeded) const
{
  if (m_nowActiveTag && m_nowActiveTag->IsActive())
  {
    return m_nowActiveTag;
  }

  if (bUpdateIfNeeded)
  {
    for (const auto& tag : m_changedTags)
    {
      if (tag.second->IsActive())
      {
        m_nowActiveTag = tag.second;
        return tag.second;
      }
    }

    if (m_database)
    {
      const CDateTime activeTime =
          CServiceBroker::GetPVRManager().PlaybackState()->GetPlaybackTime();
      const std::vector<std::shared_ptr<CPVREpgInfoTag>> tags = CreateEntries(
          m_database->GetEpgTagsByMinEndMaxStartTime(m_iEpgID, activeTime, activeTime));
      if (!tags.empty())
      {
        if (tags.size() > 1)
          CLog::LogF(LOGWARNING, "Got multiple results. Picking up the first.");

        m_nowActiveTag = tags.front();
        return m_nowActiveTag;
      }
    }
  }

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetPreviouslyEndedTag() const
{
  if (!m_previouslyEndedTag || !m_previouslyEndedTag->WasActive())
  {
    if (m_database)
    {
      const CDateTime activeTime =
          CServiceBroker::GetPVRManager().PlaybackState()->GetPlaybackTime();
      m_previouslyEndedTag =
          CreateEntry(m_database->GetEpgTagByMaxEndTime(m_iEpgID, activeTime - ONE_SECOND));
    }

    for (auto it = m_changedTags.rbegin(); it != m_changedTags.rend(); ++it)
    {
      if (it->second->WasActive())
      {
        if (!m_previouslyEndedTag || m_previouslyEndedTag->EndAsUTC() < it->second->EndAsUTC())
        {
          m_previouslyEndedTag = it->second;
          break;
        }
      }
    }
  }

  return m_previouslyEndedTag;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::GetNextUpcomingTag() const
{
  if (!m_nextUpcomingTag || !m_nextUpcomingTag->IsUpcoming())
  {
    if (m_database)
    {
      const CDateTime activeTime =
          CServiceBroker::GetPVRManager().PlaybackState()->GetPlaybackTime();
      m_nextUpcomingTag =
          CreateEntry(m_database->GetEpgTagByMinStartTime(m_iEpgID, activeTime + ONE_SECOND));
    }

    for (const auto& tag : m_changedTags)
    {
      if (tag.second->IsUpcoming())
      {
        if (!m_nextUpcomingTag || m_nextUpcomingTag->StartAsUTC() > tag.second->StartAsUTC())
        {
          m_nextUpcomingTag = tag.second;
          break;
        }
      }
    }
  }

  return m_nextUpcomingTag;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgTagsContainer::CreateGapTag(const CDateTime& start,
                                                                   const CDateTime& end) const
{
  return std::make_shared<CPVREpgInfoTag>(m_channelData, m_iEpgID, start, end, true);
}

namespace
{

void ResolveConflictingTags(const std::shared_ptr<CPVREpgInfoTag>& changedTag,
                            std::vector<std::shared_ptr<CPVREpgInfoTag>>& tags)
{
  const CDateTime changedTagStart = changedTag->StartAsUTC();
  const CDateTime changedTagEnd = changedTag->EndAsUTC();

  for (auto it = tags.begin(); it != tags.end();)
  {
    bool bInsert = false;

    if (changedTagEnd > (*it)->StartAsUTC() && changedTagStart < (*it)->EndAsUTC())
    {
      it = tags.erase(it);

      if (it == tags.end())
      {
        bInsert = true;
      }
    }
    else if ((*it)->StartAsUTC() >= changedTagEnd)
    {
      bInsert = true;
    }
    else
    {
      ++it;
    }

    if (bInsert)
    {
      tags.emplace(it, changedTag);
      break;
    }
  }
}

} // unnamed namespace

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgTagsContainer::GetTimeline(
    const CDateTime& timelineStart,
    const CDateTime& timelineEnd,
    const CDateTime& minEventEnd,
    const CDateTime& maxEventStart) const
{
  if (m_database)
  {
    std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;

    if (!m_changedTags.empty() && !m_database->GetFirstStartTime(m_iEpgID).IsValid())
    {
      // nothing in the db yet. take what we have in memory.
      for (const auto& tag : m_changedTags)
      {
        if (tag.second->EndAsUTC() > minEventEnd && tag.second->StartAsUTC() <= maxEventStart)
          tags.emplace_back(tag.second);
      }

      if (!tags.empty())
        FixOverlappingEvents(tags);
    }
    else
    {
      tags = m_database->GetEpgTagsByMinEndMaxStartTime(m_iEpgID, minEventEnd, maxEventStart);

      if (!m_changedTags.empty())
      {
        // Fix data inconsistencies
        for (const auto& changedTag : m_changedTags)
        {
          if (changedTag.second->EndAsUTC() > minEventEnd &&
              changedTag.second->StartAsUTC() <= maxEventStart)
          {
            // tag is in queried range, thus it could cause inconsistencies...
            ResolveConflictingTags(changedTag.second, tags);
          }
        }
      }
    }

    tags = CreateEntries(tags);

    std::vector<std::shared_ptr<CPVREpgInfoTag>> result;

    for (const auto& epgTag : tags)
    {
      if (!result.empty())
      {
        const CDateTime currStart = epgTag->StartAsUTC();
        const CDateTime prevEnd = result.back()->EndAsUTC();
        if ((currStart - prevEnd) > ONE_SECOND)
        {
          // insert gap tag before current tag
          result.emplace_back(CreateGapTag(prevEnd, currStart - ONE_SECOND));
        }
      }

      result.emplace_back(epgTag);
    }

    if (result.empty())
    {
      // create single gap tag
      CDateTime maxEnd = m_database->GetMaxEndTime(m_iEpgID, minEventEnd);
      if (!maxEnd.IsValid() || maxEnd < timelineStart)
        maxEnd = timelineStart;

      CDateTime minStart = m_database->GetMinStartTime(m_iEpgID, maxEventStart);
      if (!minStart.IsValid() || minStart > timelineEnd)
        minStart = timelineEnd;
      else
        minStart -= ONE_SECOND;

      result.emplace_back(CreateGapTag(maxEnd, minStart));
    }
    else
    {
      if (result.front()->StartAsUTC() > minEventEnd)
      {
        // prepend gap tag
        CDateTime maxEnd = m_database->GetMaxEndTime(m_iEpgID, minEventEnd);
        if (!maxEnd.IsValid() || maxEnd < timelineStart)
          maxEnd = timelineStart;

        result.insert(result.begin(),
                      CreateGapTag(maxEnd, result.front()->StartAsUTC() - ONE_SECOND));
      }

      if (result.back()->EndAsUTC() < maxEventStart)
      {
        // append gap tag
        CDateTime minStart = m_database->GetMinStartTime(m_iEpgID, maxEventStart);
        if (!minStart.IsValid() || minStart > timelineEnd)
          minStart = timelineEnd;
        else
          minStart -= ONE_SECOND;

        result.emplace_back(CreateGapTag(result.back()->EndAsUTC(), minStart));
      }
    }

    return result;
  }

  return {};
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgTagsContainer::GetAllTags() const
{
  if (m_database)
  {
    std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;
    if (!m_changedTags.empty() && !m_database->GetFirstStartTime(m_iEpgID).IsValid())
    {
      // nothing in the db yet. take what we have in memory.
      for (const auto& tag : m_changedTags)
        tags.emplace_back(tag.second);

      if (!tags.empty())
        FixOverlappingEvents(tags);
    }
    else
    {
      tags = m_database->GetAllEpgTags(m_iEpgID);

      if (!m_changedTags.empty())
      {
        // Fix data inconsistencies
        for (const auto& changedTag : m_changedTags)
        {
          ResolveConflictingTags(changedTag.second, tags);
        }
      }
    }

    return CreateEntries(tags);
  }

  return {};
}

CDateTime CPVREpgTagsContainer::GetFirstStartTime() const
{
  CDateTime result;

  if (!m_changedTags.empty())
    result = (*m_changedTags.cbegin()).second->StartAsUTC();

  if (m_database)
  {
    const CDateTime dbResult = m_database->GetFirstStartTime(m_iEpgID);
    if (!result.IsValid() || (dbResult.IsValid() && dbResult < result))
      result = dbResult;
  }

  return result;
}

CDateTime CPVREpgTagsContainer::GetLastEndTime() const
{
  CDateTime result;

  if (!m_changedTags.empty())
    result = (*m_changedTags.crbegin()).second->EndAsUTC();

  if (m_database)
  {
    const CDateTime dbResult = m_database->GetLastEndTime(m_iEpgID);
    if (result.IsValid() || (dbResult.IsValid() && dbResult > result))
      result = dbResult;
  }

  return result;
}

bool CPVREpgTagsContainer::NeedsSave() const
{
  return !m_changedTags.empty() || !m_deletedTags.empty();
}

void CPVREpgTagsContainer::Persist(bool bCommit)
{
  if (m_database)
  {
    m_database->Lock();

    CLog::Log(LOGNOTICE, "EPG Tags Container: Updating %d, deleting %d events...",
              m_changedTags.size(), m_deletedTags.size());

    for (const auto& tag : m_deletedTags)
      m_database->Delete(*tag.second);

    m_deletedTags.clear();

    for (const auto& tag : m_changedTags)
    {
      // remove any conflicting events from database before persisting the new event
      m_database->DeleteEpgTagsByMinEndMaxStartTime(m_iEpgID, tag.second->StartAsUTC(),
                                                    tag.second->EndAsUTC() - ONE_SECOND);

      tag.second->Persist(m_database, false);
    }

    m_changedTags.clear();

    if (bCommit)
      m_database->CommitInsertQueries();

    m_database->Unlock();
  }
}
