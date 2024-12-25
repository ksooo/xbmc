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
#include "utils/PlayerUtils.h"
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

CVideoPlayActionProcessor::CVideoPlayActionProcessor(const std::shared_ptr<CFileItem>& item)
  : m_item(item), m_stackParts(std::make_unique<CFileItemList>())
{
}

CVideoPlayActionProcessor::~CVideoPlayActionProcessor() = default;

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

    m_chosenStackPart = ChooseStackPart();
    if (m_chosenStackPart < 1)
    {
      m_userCancelled = true;
      return true; // User cancelled the select menu. We're done.
    }
  }

  return Process(action);
}

bool CVideoPlayActionProcessor::Process(Action action)
{
  switch (action)
  {
    case ACTION_PLAY_OR_RESUME:
    {
      const Action selectedAction = ChoosePlayOrResume();
      if (selectedAction < 0)
      {
        m_userCancelled = true;
        return true; // User cancelled the select menu. We're done.
      }

      return Process(selectedAction);
    }

    case ACTION_RESUME:
    {
      SetResumeData();
      return OnResumeSelected();
    }

    case ACTION_PLAY_FROM_BEGINNING:
    {
      SetStartData();
      return OnPlaySelected();
    }

    default:
      break;
  }
  return false; // We did not handle the action.
}

Action CVideoPlayActionProcessor::ChoosePlayOrResume()
{
  if (m_chosenStackPart &&
      VIDEO::UTILS::GetStackPartResumeOffset(*m_item, *m_stackParts, m_chosenStackPart) <= 0)
    return ACTION_PLAY_FROM_BEGINNING;

  return ChoosePlayOrResume(*m_item);
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

unsigned int CVideoPlayActionProcessor::ChooseStackPart() const
{
  XFILE::CDirectory::GetDirectory(m_item->GetDynPath(), *m_stackParts, "",
                                  XFILE::DIR_FLAG_DEFAULTS);

  if (m_stackParts->IsEmpty())
  {
    CLog::LogF(LOGERROR, "Invalid item (empty stack)!");
    return 0; // done
  }

  auto& parts{*m_stackParts};
  for (int i = 0; i < parts.Size(); ++i)
  {
    parts[i]->SetLabel(StringUtils::Format(g_localizeStrings.Get(23051), i + 1)); // Part #
  }

  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);

  dialog->Reset();
  dialog->SetHeading(CVariant{20324}); // Play part...
  dialog->SetItems(*m_stackParts);
  dialog->Open();

  if (!dialog->IsConfirmed())
    return 0; // User cancelled the dialog.

  return dialog->GetSelectedItem() + 1; // part numbers are 1-based
}

void CVideoPlayActionProcessor::SetResumeData()
{
  if (m_chosenStackPart)
  {
    m_item->m_lStartPartNumber = m_chosenStackPart;
    m_item->SetStartOffset(
        VIDEO::UTILS::GetStackPartResumeOffset(*m_item, *m_stackParts, m_chosenStackPart));
  }
  else
  {
    m_item->m_lStartPartNumber = 1;
    m_item->SetStartOffset(STARTOFFSET_RESUME);
  }
}

void CVideoPlayActionProcessor::SetStartData()
{
  if (m_chosenStackPart)
  {
    m_item->m_lStartPartNumber = m_chosenStackPart;
    m_item->SetStartOffset(
        VIDEO::UTILS::GetStackPartStartOffset(*m_item, *m_stackParts, m_chosenStackPart));
  }
  else
  {
    m_item->m_lStartPartNumber = 1;
    m_item->SetStartOffset(0);
  }
}

bool CVideoPlayActionProcessor::OnResumeSelected()
{
  Play("");
  return true;
}

bool CVideoPlayActionProcessor::OnPlaySelected()
{
  std::string player;
  if (m_choosePlayer)
  {
    const std::vector<std::string> players{CPlayerUtils::GetPlayersForItem(*m_item)};
    const CPlayerCoreFactory& playerCoreFactory{CServiceBroker::GetPlayerCoreFactory()};
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
