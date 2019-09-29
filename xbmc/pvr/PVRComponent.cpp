/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRComponent.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonInfo.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/dialogs/GUIDialogPVRChannelGuide.h"
#include "pvr/dialogs/GUIDialogPVRChannelManager.h"
#include "pvr/dialogs/GUIDialogPVRChannelsOSD.h"
#include "pvr/dialogs/GUIDialogPVRClientPriorities.h"
#include "pvr/dialogs/GUIDialogPVRGroupManager.h"
#include "pvr/dialogs/GUIDialogPVRGuideControls.h"
#include "pvr/dialogs/GUIDialogPVRGuideInfo.h"
#include "pvr/dialogs/GUIDialogPVRGuideSearch.h"
#include "pvr/dialogs/GUIDialogPVRRadioRDSInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingSettings.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"
#include "pvr/windows/GUIViewStatePVR.h"
#include "pvr/windows/GUIWindowPVRChannels.h"
#include "pvr/windows/GUIWindowPVRGuide.h"
#include "pvr/windows/GUIWindowPVRRecordings.h"
#include "pvr/windows/GUIWindowPVRSearch.h"
#include "pvr/windows/GUIWindowPVRTimerRules.h"
#include "pvr/windows/GUIWindowPVRTimers.h"
#include "settings/AdvancedSettings.h"

using namespace PVR;

CPVRComponent::CPVRComponent() = default;

CPVRComponent::~CPVRComponent() = default;

////////////////////////////////////////////////////
// services
////////////////////////////////////////////////////

void CPVRComponent::Stop()
{
  CServiceBroker::GetPVRManager().Stop();
}

void CPVRComponent::Restart()
{
  CServiceBroker::GetPVRManager().Init();
}

bool CPVRComponent::IsStarted() const
{
  return CServiceBroker::GetPVRManager().IsStarted();
}

////////////////////////////////////////////////////
// addons
////////////////////////////////////////////////////

std::shared_ptr<ADDON::IAddon> CPVRComponent::CreatePVRAddonInstance(const std::shared_ptr<ADDON::CAddonInfo>& info)
{
  return std::make_shared<CPVRClient>(info);
}

void* CPVRComponent::GetPVRAddonInstanceInterface(ADDON::IAddon* addon) const
{
  CPVRClient* client = dynamic_cast<CPVRClient*>(addon);
  if (!client)
    return nullptr;

  return client->GetInstanceInterface();
}

////////////////////////////////////////////////////
// databases
////////////////////////////////////////////////////

std::vector<CPVRComponent::Database> CPVRComponent::GetDatabases(const std::shared_ptr<CAdvancedSettings>& advancedSettings) const
{
  return {{std::make_shared<CPVRDatabase>(), &advancedSettings->m_databaseTV},
          {std::make_shared<CPVREpgDatabase>(), &advancedSettings->m_databaseEpg}};
}

////////////////////////////////////////////////////
// GUI lib
////////////////////////////////////////////////////

void CPVRComponent::CreateWindowsAndDialogs(CGUIWindowManager* guiManager)
{
  if (!guiManager)
    return;

  guiManager->Add(new CGUIWindowPVRTVChannels);
  guiManager->Add(new CGUIWindowPVRTVRecordings);
  guiManager->Add(new CGUIWindowPVRTVGuide);
  guiManager->Add(new CGUIWindowPVRTVTimers);
  guiManager->Add(new CGUIWindowPVRTVTimerRules);
  guiManager->Add(new CGUIWindowPVRTVSearch);
  guiManager->Add(new CGUIWindowPVRRadioChannels);
  guiManager->Add(new CGUIWindowPVRRadioRecordings);
  guiManager->Add(new CGUIWindowPVRRadioGuide);
  guiManager->Add(new CGUIWindowPVRRadioTimers);
  guiManager->Add(new CGUIWindowPVRRadioTimerRules);
  guiManager->Add(new CGUIWindowPVRRadioSearch);

  guiManager->Add(new CGUIDialogPVRRadioRDSInfo);
  guiManager->Add(new CGUIDialogPVRGuideInfo);
  guiManager->Add(new CGUIDialogPVRRecordingInfo);
  guiManager->Add(new CGUIDialogPVRTimerSettings);
  guiManager->Add(new CGUIDialogPVRGroupManager);
  guiManager->Add(new CGUIDialogPVRChannelManager);
  guiManager->Add(new CGUIDialogPVRGuideSearch);
  guiManager->Add(new CGUIDialogPVRChannelsOSD);
  guiManager->Add(new CGUIDialogPVRChannelGuide);
  guiManager->Add(new CGUIDialogPVRRecordingSettings);
  guiManager->Add(new CGUIDialogPVRClientPriorities);
  guiManager->Add(new CGUIDialogPVRGuideControls);
}

void CPVRComponent::DestroyWindowsAndDialogs(CGUIWindowManager* guiManager)
{
  if (!guiManager)
    return;

  //! @todo Create uses class names, destroy uses ids. Fix this inconsistency.

  guiManager->DestroyWindow(WINDOW_TV_CHANNELS);
  guiManager->DestroyWindow(WINDOW_TV_RECORDINGS);
  guiManager->DestroyWindow(WINDOW_TV_GUIDE);
  guiManager->DestroyWindow(WINDOW_TV_TIMERS);
  guiManager->DestroyWindow(WINDOW_TV_TIMER_RULES);
  guiManager->DestroyWindow(WINDOW_TV_SEARCH);
  guiManager->DestroyWindow(WINDOW_RADIO_CHANNELS);
  guiManager->DestroyWindow(WINDOW_RADIO_RECORDINGS);
  guiManager->DestroyWindow(WINDOW_RADIO_GUIDE);
  guiManager->DestroyWindow(WINDOW_RADIO_TIMERS);
  guiManager->DestroyWindow(WINDOW_RADIO_TIMER_RULES);
  guiManager->DestroyWindow(WINDOW_RADIO_SEARCH);

  guiManager->DestroyWindow(WINDOW_DIALOG_PVR_RADIO_RDS_INFO);
  guiManager->DestroyWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
  guiManager->DestroyWindow(WINDOW_DIALOG_PVR_RECORDING_INFO);
  guiManager->DestroyWindow(WINDOW_DIALOG_PVR_TIMER_SETTING);
  guiManager->DestroyWindow(WINDOW_DIALOG_PVR_GROUP_MANAGER);
  guiManager->DestroyWindow(WINDOW_DIALOG_PVR_CHANNEL_MANAGER);
  guiManager->DestroyWindow(WINDOW_DIALOG_PVR_GUIDE_SEARCH);
  guiManager->DestroyWindow(WINDOW_DIALOG_PVR_OSD_CHANNELS);
  guiManager->DestroyWindow(WINDOW_DIALOG_PVR_CHANNEL_GUIDE);
  guiManager->DestroyWindow(WINDOW_DIALOG_PVR_RECORDING_SETTING);
  guiManager->DestroyWindow(WINDOW_DIALOG_PVR_CLIENT_PRIORITIES);
  guiManager->DestroyWindow(WINDOW_DIALOG_PVR_GUIDE_CONTROLS);
}

int CPVRComponent::GetFullscreenWindowID(const CFileItem& item) const
{
  if (item.HasPVRChannelInfoTag())
  {
    bool bRadio = item.GetPVRChannelInfoTag()->IsRadio();

    if (CServiceBroker::GetPVRManager().GUIActions()->GetChannelNumberInputHandler().HasChannelNumber())
      return bRadio ? WINDOW_FULLSCREEN_RADIO_INPUT : WINDOW_FULLSCREEN_LIVETV_INPUT;
    else if (CServiceBroker::GetPVRManager().GUIActions()->GetChannelNavigator().IsPreview())
      return bRadio ? WINDOW_FULLSCREEN_RADIO_PREVIEW : WINDOW_FULLSCREEN_LIVETV_PREVIEW;
    else
      return bRadio ? WINDOW_FULLSCREEN_RADIO : WINDOW_FULLSCREEN_LIVETV;
  }
  else
    return WINDOW_INVALID;
}

CGUIViewState* CPVRComponent::GetViewState(int windowId, const CFileItemList& items) const
{
  CGUIViewState* state = nullptr;

  switch (windowId)
  {
    case WINDOW_RADIO_CHANNELS:
    case WINDOW_TV_CHANNELS:
      state = new CGUIViewStateWindowPVRChannels(windowId, items);
      break;
    case WINDOW_RADIO_RECORDINGS:
    case WINDOW_TV_RECORDINGS:
      state = new CGUIViewStateWindowPVRRecordings(windowId, items);
      break;
    case WINDOW_RADIO_GUIDE:
    case WINDOW_TV_GUIDE:
      state = new CGUIViewStateWindowPVRGuide(windowId, items);
      break;
    case WINDOW_RADIO_TIMERS:
    case WINDOW_TV_TIMERS:
    case WINDOW_RADIO_TIMER_RULES:
    case WINDOW_TV_TIMER_RULES:
      state = new CGUIViewStateWindowPVRTimers(windowId, items);
      break;
    case WINDOW_RADIO_SEARCH:
    case WINDOW_TV_SEARCH:
      state = new CGUIViewStateWindowPVRSearch(windowId, items);
      break;
    default:
      break;
  }

  return state;
}

void CPVRComponent::OnLocaleChanged()
{
  CServiceBroker::GetPVRManager().LocalizationChanged();
}

////////////////////////////////////////////////////
// power management
////////////////////////////////////////////////////

void CPVRComponent::OnSleep()
{
  CServiceBroker::GetPVRManager().OnSleep();
}

void CPVRComponent::OnWake()
{
  CServiceBroker::GetPVRManager().OnWake();
}

bool CPVRComponent::CanSystemPowerdown(bool bAskUser /*= true*/) const
{
  return CServiceBroker::GetPVRManager().GUIActions()->CanSystemPowerdown(bAskUser);
}

////////////////////////////////////////////////////
// media playback
////////////////////////////////////////////////////

bool CPVRComponent::PlayMedia(const std::shared_ptr<CFileItem>& item) const
{
  return CServiceBroker::GetPVRManager().GUIActions()->PlayMedia(item);
}

void CPVRComponent::OnPlaybackStarted(const std::shared_ptr<CFileItem>& item)
{
  CServiceBroker::GetPVRManager().OnPlaybackStarted(item);
}

void CPVRComponent::OnPlaybackStopped(const std::shared_ptr<CFileItem>& item)
{
  CServiceBroker::GetPVRManager().OnPlaybackStopped(item);
}

void CPVRComponent::OnPlaybackEnded(const std::shared_ptr<CFileItem>& item)
{
  CServiceBroker::GetPVRManager().OnPlaybackEnded(item);
}

////////////////////////////////////////////////////
// channels
////////////////////////////////////////////////////

bool CPVRComponent::IsPVRChannel(const std::string& uri) const
{
  return CPVRChannelsPath(uri).IsChannel();
}

////////////////////////////////////////////////////
// channel groups
////////////////////////////////////////////////////

bool CPVRComponent::IsPVRChannelGroup(const std::string& uri) const
{
  return CPVRChannelsPath(uri).IsChannelGroup();
}

////////////////////////////////////////////////////
// recordings
////////////////////////////////////////////////////

bool CPVRComponent::ResetRecordingResumePoint(const std::shared_ptr<CFileItem>& recording)
{
  return recording &&
         recording->HasPVRRecordingInfoTag() &&
         CServiceBroker::GetPVRManager().Recordings()->ResetResumePoint(recording->GetPVRRecordingInfoTag());
}

bool CPVRComponent::MarkRecordingWatched(const std::shared_ptr<CFileItem>& recording, bool bWatched)
{
  return recording &&
         recording->HasPVRRecordingInfoTag() &&
         CServiceBroker::GetPVRManager().Recordings()->MarkWatched(recording->GetPVRRecordingInfoTag(), bWatched);
}
