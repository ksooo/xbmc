/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupPlayedChannels.h"

#include "guilib/LocalizeStrings.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"

#include <algorithm>

using namespace PVR;

std::vector<std::shared_ptr<CPVRChannelGroup>> CPVRChannelGroupPlayedChannels::CreateMissingGroups(
    const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup,
    const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups)
{
  std::vector<std::shared_ptr<CPVRChannelGroup>> addedGroups;

  // This group is a singleton. So, just look if there is already an instance. If not, create one.
  const auto it =
      std::find_if(allChannelGroups.cbegin(), allChannelGroups.cend(), [](const auto& group)
                   { return group->GroupType() == PVR_GROUP_TYPE_SYSTEM_PLAYED_CHANNELS; });
  if (it == allChannelGroups.cend())
  {
    const std::string groupName{g_localizeStrings.Get(allChannelsGroup->IsRadio() ? 858 : 857)};
    const CPVRChannelsPath path{allChannelsGroup->GetChannelType(), groupName,
                                PVR_GROUP_CLIENT_ID_LOCAL};
    const std::shared_ptr<CPVRChannelGroup> newGroup{
        std::make_shared<CPVRChannelGroupPlayedChannels>(path, allChannelsGroup)};
    newGroup->SetHidden(true); // Hide group by default
    addedGroups.emplace_back(newGroup);
  }

  return addedGroups;
}

bool CPVRChannelGroupPlayedChannels::UpdateGroupMembers(
    const std::shared_ptr<CPVRChannelGroup>& allChannelsGroup,
    const std::vector<std::shared_ptr<CPVRChannelGroup>>& allChannelGroups)
{
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers;

  // Collect and populate matching members.
  for (const auto& member : allChannelsGroup->GetMembers())
  {
    // Remove never played channels.
    if (!member->Channel()->LastWatched())
      continue;

    groupMembers.emplace_back(std::make_shared<CPVRChannelGroupMember>(
        GroupID(), GroupName(), GetClientID(), member->Channel()));
  }

  return UpdateGroupEntries(groupMembers);
}
