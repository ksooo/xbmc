/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoPlayActionProcessor.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "playlists/PlayListTypes.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoUtils.h"
#include "video/guilib/VideoGUIUtils.h"
#include "video/guilib/VideoVersionHelper.h"

namespace KODI::VIDEO::GUILIB
{

Action CVideoPlayActionProcessor::GetDefaultAction()
{
  return static_cast<Action>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_MYVIDEOS_PLAYACTION));
}

bool CVideoPlayActionProcessor::ProcessDefaultAction()
{
  return ProcessAction(GetDefaultAction());
}

bool CVideoPlayActionProcessor::ProcessAction(Action action)
{
  m_userCancelled = false;

  const auto movie{CVideoVersionHelper::ChooseVideoFromAssets(m_item)};
  if (movie)
    m_item = movie;
  else
  {
    m_userCancelled = true;
    return true; // User cancelled the select menu. We're done.
  }

  if (m_chooseStackPart)
  {
    if (!URIUtils::IsStack(m_item->GetDynPath()))
    {
      CLog::LogF(LOGERROR, "Invalid item (not a stack)!");
      return true; // done
    }

    CFileItemList parts;
    const unsigned int partNumber{ChooseStackPart(parts)};
    if (partNumber < 1)
    {
      m_userCancelled = true;
      return true; // User cancelled the select menu. We're done.
    }

    const VIDEO::UTILS::ResumeInformation resumeInfo{
        VIDEO::UTILS::GetStackPartResumeInformation(*m_item, parts, partNumber)};
    m_item->SetStartOffset(resumeInfo.startOffset);
    m_item->m_lStartPartNumber = partNumber;
  }

  return Process(action);
}

bool CVideoPlayActionProcessor::Process(Action action)
{
  switch (action)
  {
    case ACTION_PLAY_OR_RESUME:
    {
      const Action selectedAction = ChoosePlayOrResume(*m_item);
      if (selectedAction < 0)
      {
        m_userCancelled = true;
        return true; // User cancelled the select menu. We're done.
      }

      return Process(selectedAction);
    }

    case ACTION_RESUME:
      return OnResumeSelected();

    case ACTION_PLAY_FROM_BEGINNING:
      return OnPlaySelected();

    default:
      break;
  }
  return false; // We did not handle the action.
}

Action CVideoPlayActionProcessor::ChoosePlayOrResume(const CFileItem& item)
{
  Action action = ACTION_PLAY_FROM_BEGINNING;

  const std::string resumeString = VIDEO::UTILS::GetResumeString(item);
  if (!resumeString.empty())
  {
    CContextButtons choices;

    choices.Add(ACTION_RESUME, resumeString);
    choices.Add(ACTION_PLAY_FROM_BEGINNING, 12021); // Play from beginning

    action = static_cast<Action>(CGUIDialogContextMenu::ShowAndGetChoice(choices));
  }

  return action;
}

unsigned int CVideoPlayActionProcessor::ChooseStackPart(CFileItemList& parts) const
{
  XFILE::CDirectory::GetDirectory(m_item->GetDynPath(), parts, "", XFILE::DIR_FLAG_DEFAULTS);

  for (int i = 0; i < parts.Size(); ++i)
    parts[i]->SetLabel(StringUtils::Format(g_localizeStrings.Get(23051), i + 1)); // Part #

  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);

  dialog->Reset();
  dialog->SetHeading(CVariant{20324}); // Play part...
  dialog->SetItems(parts);
  dialog->Open();

  if (!dialog->IsConfirmed())
    return 0; // User cancelled the dialog.

  return dialog->GetSelectedItem() + 1; // part numbers are 1-based
}

bool CVideoPlayActionProcessor::OnResumeSelected()
{
  m_item->SetStartOffset(STARTOFFSET_RESUME);
  Play("");
  return true;
}

namespace
{
std::vector<std::string> GetPlayers(const CPlayerCoreFactory& playerCoreFactory,
                                    const CFileItem& item)
{
  std::vector<std::string> players;
  if (VIDEO::IsVideoDb(item))
  {
    //! @todo CPlayerCoreFactory and classes called from there do not handle dyn path correctly.
    CFileItem item2{item};
    item2.SetPath(item.GetDynPath());
    playerCoreFactory.GetPlayers(item2, players);
  }
  else
    playerCoreFactory.GetPlayers(item, players);

  return players;
}
} // unnamed namespace

bool CVideoPlayActionProcessor::OnPlaySelected()
{
  std::string player;
  if (m_choosePlayer)
  {
    const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};
    const std::vector<std::string> players{GetPlayers(playerCoreFactory, *m_item)};
    player = playerCoreFactory.SelectPlayerDialog(players);
    if (player.empty())
    {
      m_userCancelled = true;
      return true; // User cancelled player selection. We're done.
    }
  }

  Play(player);
  return true;
}

void CVideoPlayActionProcessor::Play(const std::string& player)
{
  auto item{m_item};
  if (item->m_bIsFolder && item->HasVideoVersions())
  {
    //! @todo get rid of "videos with versions as folder" hack!
    item = std::make_shared<CFileItem>(*item);
    item->m_bIsFolder = false;
  }

  item->SetProperty("playlist_type_hint", static_cast<int>(KODI::PLAYLIST::Id::TYPE_VIDEO));
  const ContentUtils::PlayMode mode{item->GetProperty("CheckAutoPlayNextItem").asBoolean()
                                        ? ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM
                                        : ContentUtils::PlayMode::PLAY_ONLY_THIS};
  VIDEO::UTILS::PlayItem(item, player, mode);
}

} // namespace KODI::VIDEO::GUILIB
