/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/GUIControlsGUIInfo.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogProgress.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUITextBox.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/IGUIContainer.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoHelper.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "music/dialogs/GUIDialogSongInfo.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoTag.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "view/GUIViewState.h"
#include "windows/GUIMediaWindow.h"

using namespace KODI::GUILIB;
using namespace KODI::GUILIB::GUIINFO;

void CGUIControlsGUIInfo::SetContainerMoving(int id, bool next, bool scrolling)
{
  // magnitude 2 indicates a scroll, sign indicates direction
  m_containerMoves[id] = (next ? 1 : -1) * (scrolling ? 2 : 1);
}

void CGUIControlsGUIInfo::ResetContainerMovingCache()
{
  m_containerMoves.clear();
}

bool CGUIControlsGUIInfo::InitCurrentItem(CFileItem *item)
{
  return false;
}

bool CGUIControlsGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  switch (info.GetInfo())
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // CONTAINER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case CONTAINER_FOLDERPATH:
    case CONTAINER_FOLDERNAME:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        if (info.GetInfo() == CONTAINER_FOLDERNAME)
          value = window->CurrentDirectory().GetLabel();
        else
          value = CURL(window->CurrentDirectory().GetPath()).GetWithoutUserDetails();
        return true;
      }
      break;
    }
    case CONTAINER_PLUGINNAME:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        const CURL url(window->CurrentDirectory().GetPath());
        if (url.IsProtocol("plugin"))
        {
          value = URIUtils::GetFileName(url.GetHostName());
          return true;
        }
      }
      break;
    }
    case CONTAINER_VIEWCOUNT:
    case CONTAINER_VIEWMODE:
    {
      CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        const CGUIControl *control = window->GetControl(window->GetViewContainerID());
        if (control && control->IsContainer())
        {
          if (info.GetInfo() == CONTAINER_VIEWMODE)
          {
            value = static_cast<const IGUIContainer*>(control)->GetLabel();
            return true;
          }
          else if (info.GetInfo() == CONTAINER_VIEWCOUNT)
          {
            value = std::to_string(window->GetViewCount());
            return true;
          }
        }
      }
      break;
    }
    case CONTAINER_SORT_METHOD:
    case CONTAINER_SORT_ORDER:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        const CGUIViewState *viewState = window->GetViewState();
        if (viewState)
        {
          if (info.GetInfo() == CONTAINER_SORT_METHOD)
          {
            value = g_localizeStrings.Get(viewState->GetSortMethodLabel());
            return true;
          }
          else if (info.GetInfo() == CONTAINER_SORT_ORDER)
          {
            value = g_localizeStrings.Get(viewState->GetSortOrderLabel());
            return true;
          }
        }
      }
      break;
    }
    case CONTAINER_PROPERTY:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CurrentDirectory().GetProperty(info.GetData3()).asString();
        return true;
      }
      break;
    }
    case CONTAINER_ART:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CurrentDirectory().GetArt(info.GetData3());
        return true;
      }
      break;
    }
    case CONTAINER_CONTENT:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CurrentDirectory().GetContent();
        return true;
      }
      break;
    }
    case CONTAINER_SHOWPLOT:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CurrentDirectory().GetProperty("showplot").asString();
        return true;
      }
      break;
    }
    case CONTAINER_SHOWTITLE:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CurrentDirectory().GetProperty("showtitle").asString();
        return true;
      }
      break;
    }
    case CONTAINER_PLUGINCATEGORY:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CurrentDirectory().GetProperty("plugincategory").asString();
        return true;
      }
      break;
    }
    case CONTAINER_TOTALTIME:
    case CONTAINER_TOTALWATCHED:
    case CONTAINER_TOTALUNWATCHED:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        int count = 0;
        const CFileItemList& items = window->CurrentDirectory();
        for (const auto& i : items)
        {
          // Iterate through container and count watched, unwatched and total duration.
          if (info.GetInfo() == CONTAINER_TOTALWATCHED && i->HasVideoInfoTag() &&
              i->GetVideoInfoTag()->GetPlayCount() > 0)
            count += 1;
          else if (info.GetInfo() == CONTAINER_TOTALUNWATCHED && i->HasVideoInfoTag() &&
                   i->GetVideoInfoTag()->GetPlayCount() == 0)
            count += 1;
          else if (info.GetInfo() == CONTAINER_TOTALTIME && i->HasMusicInfoTag())
            count += i->GetMusicInfoTag()->GetDuration();
          else if (info.GetInfo() == CONTAINER_TOTALTIME && i->HasVideoInfoTag())
            count += i->GetVideoInfoTag()->m_streamDetails.GetVideoDuration();
        }
        if (info.GetInfo() == CONTAINER_TOTALTIME && count > 0)
        {
          value = StringUtils::SecondsToTimeString(count);
          return true;
        }
        else if (info.GetInfo() == CONTAINER_TOTALWATCHED ||
                 info.GetInfo() == CONTAINER_TOTALUNWATCHED)
        {
          value = std::to_string(count);
          return true;
        }
      }
      break;
    }
    case CONTAINER_NUM_PAGES:
    case CONTAINER_CURRENT_PAGE:
    case CONTAINER_NUM_ITEMS:
    case CONTAINER_POSITION:
    case CONTAINER_ROW:
    case CONTAINER_COLUMN:
    case CONTAINER_CURRENT_ITEM:
    case CONTAINER_NUM_ALL_ITEMS:
    case CONTAINER_NUM_NONFOLDER_ITEMS:
    {
      const CGUIControl *control = nullptr;
      if (info.GetData1())
      { // container specified
        CGUIWindow* window{GUIINFO::GetWindow(contextWindow)};
        if (window)
          control = window->GetControl(info.GetData1());
      }
      else
      { // no container specified - assume a mediawindow
        CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
        if (window)
          control = window->GetControl(window->GetViewContainerID());
      }
      if (control)
      {
        if (control->IsContainer())
          value = static_cast<const IGUIContainer*>(control)->GetLabel(info.GetInfo());
        else if (control->GetControlType() == CGUIControl::GUICONTROL_GROUPLIST)
          value = static_cast<const CGUIControlGroupList*>(control)->GetLabel(info.GetInfo());
        else if (control->GetControlType() == CGUIControl::GUICONTROL_TEXTBOX)
          value = static_cast<const CGUITextBox*>(control)->GetLabel(info.GetInfo());
        return true;
      }
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // CONTROL_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case CONTROL_GET_LABEL:
    {
      CGUIWindow* window{GUIINFO::GetWindow(contextWindow)};
      if (window)
      {
        const CGUIControl *control = window->GetControl(info.GetData1());
        if (control)
        {
          int data2 = info.GetData2();
          if (data2)
            value = control->GetDescriptionByIndex(data2);
          else
            value = control->GetDescription();
          return true;
        }
      }
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // WINDOW_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case WINDOW_PROPERTY:
    {
      const CGUIWindow* window{nullptr};
      if (info.GetData1())
      { // window specified
        window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(info.GetData1());
      }
      else
      { // no window specified - assume active
        window = GUIINFO::GetWindow(contextWindow);
      }
      if (window)
      {
        value = window->GetProperty(info.GetData3()).asString();
        return true;
      }
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_CURRENT_WINDOW:
      value = g_localizeStrings.Get(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
      return true;
    case SYSTEM_STARTUP_WINDOW:
      value = std::to_string(CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
          CSettings::SETTING_LOOKANDFEEL_STARTUPWINDOW));
      return true;
    case SYSTEM_CURRENT_CONTROL:
    case SYSTEM_CURRENT_CONTROL_ID:
    {
      const CGUIWindow* window{CServiceBroker::GetGUI()->GetWindowManager().GetWindow(
          CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog())};
      if (window)
      {
        CGUIControl *control = window->GetFocusedControl();
        if (control)
        {
          if (info.GetInfo() == SYSTEM_CURRENT_CONTROL_ID)
            value = std::to_string(control->GetID());
          else if (info.GetInfo() == SYSTEM_CURRENT_CONTROL)
            value = control->GetDescription();
          return true;
        }
      }
      break;
    }
    case SYSTEM_PROGRESS_BAR:
    {
      const CGUIDialogProgress* bar{
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(
              WINDOW_DIALOG_PROGRESS)};
      if (bar && bar->IsDialogRunning())
        value = std::to_string(bar->GetPercentage());
      return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // FANART_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case FANART_COLOR1:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CurrentDirectory().GetProperty("fanart_color1").asString();
        return true;
      }
      break;
    }
    case FANART_COLOR2:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CurrentDirectory().GetProperty("fanart_color2").asString();
        return true;
      }
      break;
    }
    case FANART_COLOR3:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CurrentDirectory().GetProperty("fanart_color3").asString();
        return true;
      }
      break;
    }
    case FANART_IMAGE:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CurrentDirectory().GetArt("fanart");
        return true;
      }
      break;
    }
    default:
      break;
  }

  return false;
}

bool CGUIControlsGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.GetInfo())
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_PROGRESS_BAR:
    {
      const CGUIDialogProgress* bar{
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(
              WINDOW_DIALOG_PROGRESS)};
      if (bar && bar->IsDialogRunning())
        value = bar->GetPercentage();
      return true;
    }
    default:
      break;
  }

  return false;
}

bool CGUIControlsGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.GetInfo())
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // CONTAINER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case CONTAINER_HASFILES:
    case CONTAINER_HASFOLDERS:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        const CFileItemList& items = window->CurrentDirectory();
        for (const auto& item : items)
        {
          if ((!item->IsFolder() && info.GetInfo() == CONTAINER_HASFILES) ||
              (item->IsFolder() && !item->IsParentFolder() &&
               info.GetInfo() == CONTAINER_HASFOLDERS))
          {
            value = true;
            return true;
          }
        }
      }
      break;
    }
    case CONTAINER_STACKED:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CurrentDirectory().GetProperty("isstacked").asBoolean();
        return true;
      }
      break;
    }
    case CONTAINER_HAS_THUMB:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CurrentDirectory().HasArt("thumb");
        return true;
      }
      break;
    }
    case CONTAINER_CAN_FILTER:
    {
      CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = !window->CanFilterAdvanced();
        return true;
      }
      break;
    }
    case CONTAINER_CAN_FILTERADVANCED:
    {
      CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->CanFilterAdvanced();
        return true;
      }
      break;
    }
    case CONTAINER_FILTERED:
    {
      CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        value = window->IsFiltered();
        return true;
      }
      break;
    }
    case CONTAINER_SORT_METHOD:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        const CGUIViewState *viewState = window->GetViewState();
        if (viewState)
        {
          value = (static_cast<int>(viewState->GetSortMethod().sortBy) == info.GetData2());
          return true;
        }
      }
      break;
    }
    case CONTAINER_SORT_DIRECTION:
    {
      const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
      if (window)
      {
        const CGUIViewState *viewState = window->GetViewState();
        if (viewState)
        {
          value = (static_cast<unsigned int>(viewState->GetSortOrder()) == info.GetData1());
          return true;
        }
      }
      break;
    }
    case CONTAINER_CONTENT:
    {
      std::string content;
      CGUIWindow* window{GUIINFO::GetWindow(contextWindow)};
      if (window)
      {
        if (window->GetID() == WINDOW_DIALOG_MUSIC_INFO)
          content = static_cast<CGUIDialogMusicInfo*>(window)->GetContent();
        else if (window->GetID() == WINDOW_DIALOG_SONG_INFO)
          content = static_cast<CGUIDialogSongInfo*>(window)->GetContent();
        else if (window->GetID() == WINDOW_DIALOG_VIDEO_INFO)
          content = static_cast<CGUIDialogVideoInfo*>(window)->CurrentDirectory().GetContent();
      }
      if (content.empty())
      {
        const CGUIMediaWindow* mediaWindow{GUIINFO::GetMediaWindow(contextWindow)};
        if (mediaWindow)
          content = mediaWindow->CurrentDirectory().GetContent();
      }
      value = StringUtils::EqualsNoCase(info.GetData3(), content);
      return true;
    }
    case CONTAINER_ROW:
    case CONTAINER_COLUMN:
    case CONTAINER_POSITION:
    case CONTAINER_HAS_NEXT:
    case CONTAINER_HAS_PREVIOUS:
    case CONTAINER_SCROLLING:
    case CONTAINER_SUBITEM:
    case CONTAINER_ISUPDATING:
    case CONTAINER_HAS_PARENT_ITEM:
    {
      if (info.GetData1())
      {
        CGUIWindow* window{GUIINFO::GetWindow(contextWindow)};
        if (window)
        {
          const CGUIControl *control = window->GetControl(info.GetData1());
          if (control)
          {
            value = control->GetCondition(info.GetInfo(), info.GetData2());
            return true;
          }
        }
      }
      else
      {
        const CGUIControl *activeContainer = GUIINFO::GetActiveContainer(0, contextWindow);
        if (activeContainer)
        {
          value = activeContainer->GetCondition(info.GetInfo(), info.GetData2());
          return true;
        }
      }
      break;
    }
    case CONTAINER_HAS_FOCUS:
    { // grab our container
      CGUIWindow* window{GUIINFO::GetWindow(contextWindow)};
      if (window)
      {
        const CGUIControl *control = window->GetControl(info.GetData1());
        if (control && control->IsContainer())
        {
          const CFileItemPtr item = std::static_pointer_cast<CFileItem>(static_cast<const IGUIContainer*>(control)->GetListItem(0));
          if (item &&
              item->GetProgramCount() ==
                  info.GetData2()) //! @todo remove hack to use programcount to store item id
          {
            value = true;
            return true;
          }
        }
      }
      break;
    }
    case CONTAINER_SCROLL_PREVIOUS:
    case CONTAINER_MOVE_PREVIOUS:
    case CONTAINER_MOVE_NEXT:
    case CONTAINER_SCROLL_NEXT:
    {
      int containerId = -1;
      if (info.GetData1())
      {
        containerId = info.GetData1();
      }
      else
      {
        // no parameters, so we assume it's just requested for a media window.  It therefore
        // can only happen if the list has focus.
        const CGUIMediaWindow* window{GUIINFO::GetMediaWindow(contextWindow)};
        if (window)
          containerId = window->GetViewContainerID();
      }
      if (containerId != -1)
      {
        const std::map<int,int>::const_iterator it = m_containerMoves.find(containerId);
        if (it != m_containerMoves.end())
        {
          if (info.GetInfo() == CONTAINER_SCROLL_PREVIOUS)
            value = it->second <= -2;
          else if (info.GetInfo() == CONTAINER_MOVE_PREVIOUS)
            value = it->second <= -1;
          else if (info.GetInfo() == CONTAINER_MOVE_NEXT)
            value = it->second >= 1;
          else if (info.GetInfo() == CONTAINER_SCROLL_NEXT)
            value = it->second >= 2;
          return true;
        }
      }
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // CONTROL_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case CONTROL_IS_VISIBLE:
    {
      CGUIWindow* window{GUIINFO::GetWindow(contextWindow)};
      if (window)
      {
        // Note: This'll only work for unique id's
        const CGUIControl *control = window->GetControl(info.GetData1());
        if (control)
        {
          value = control->IsVisible();
          return true;
        }
      }
      break;
    }
    case CONTROL_IS_ENABLED:
    {
      CGUIWindow* window{GUIINFO::GetWindow(contextWindow)};
      if (window)
      {
        // Note: This'll only work for unique id's
        const CGUIControl *control = window->GetControl(info.GetData1());
        if (control)
        {
          value = !control->IsDisabled();
          return true;
        }
      }
      break;
    }
    case CONTROL_HAS_FOCUS:
    {
      const CGUIWindow* window{GUIINFO::GetWindow(contextWindow)};
      if (window)
      {
        value = (window->GetFocusedControlID() == static_cast<int>(info.GetData1()));
        return true;
      }
      break;
    }
    case CONTROL_GROUP_HAS_FOCUS:
    {
      CGUIWindow* window{GUIINFO::GetWindow(contextWindow)};
      if (window)
      {
        value = window->ControlGroupHasFocus(info.GetData1(), info.GetData2());
        return true;
      }
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // WINDOW_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case WINDOW_IS_MEDIA:
    { // note: This doesn't return true for dialogs (content, favourites, login, videoinfo)
      const CGUIWindowManager& windowMgr{CServiceBroker::GetGUI()->GetWindowManager()};
      const CGUIWindow* window{windowMgr.GetWindow(windowMgr.GetActiveWindow())};
      if (window)
      {
        value = window->IsMediaWindow();
        return true;
      }
      break;
    }
    case WINDOW_IS:
    {
      if (info.GetData1())
      {
        const CGUIWindowManager& windowMgr{CServiceBroker::GetGUI()->GetWindowManager()};
        const CGUIWindow* window{windowMgr.GetWindow(contextWindow)};
        if (!window)
        {
          // try topmost dialog
          window = windowMgr.GetWindow(windowMgr.GetTopmostModalDialog());
          if (!window)
          {
            // try active window
            window = windowMgr.GetWindow(windowMgr.GetActiveWindow());
          }
        }
        if (window)
        {
          value = (window && window->GetID() == static_cast<int>(info.GetData1()));
          return true;
        }
      }
      break;
    }
    case WINDOW_IS_VISIBLE:
    {
      if (info.GetData1())
        value = CServiceBroker::GetGUI()->GetWindowManager().IsWindowVisible(info.GetData1());
      else
        value = CServiceBroker::GetGUI()->GetWindowManager().IsWindowVisible(info.GetData3());
      return true;
    }
    case WINDOW_IS_ACTIVE:
    {
      if (info.GetData1())
        value = CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(info.GetData1());
      else
        value = CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(info.GetData3());
      return true;
    }
    case WINDOW_IS_DIALOG_TOPMOST:
    {
      if (info.GetData1())
        value = CServiceBroker::GetGUI()->GetWindowManager().IsDialogTopmost(info.GetData1());
      else
        value = CServiceBroker::GetGUI()->GetWindowManager().IsDialogTopmost(info.GetData3());
      return true;
    }
    case WINDOW_IS_MODAL_DIALOG_TOPMOST:
    {
      if (info.GetData1())
        value = CServiceBroker::GetGUI()->GetWindowManager().IsModalDialogTopmost(info.GetData1());
      else
        value = CServiceBroker::GetGUI()->GetWindowManager().IsModalDialogTopmost(info.GetData3());
      return true;
    }
    case WINDOW_NEXT:
    {
      if (info.GetData1())
      {
        value = (static_cast<int>(info.GetData1()) == m_nextWindowID);
        return true;
      }
      else
      {
        const CGUIWindow* window{
            CServiceBroker::GetGUI()->GetWindowManager().GetWindow(m_nextWindowID)};
        if (window && StringUtils::EqualsNoCase(URIUtils::GetFileName(window->GetProperty("xmlfile").asString()), info.GetData3()))
        {
          value = true;
          return true;
        }
      }
      break;
    }
    case WINDOW_PREVIOUS:
    {
      if (info.GetData1())
      {
        value = (static_cast<int>(info.GetData1()) == m_prevWindowID);
        return true;
      }
      else
      {
        const CGUIWindow* window{
            CServiceBroker::GetGUI()->GetWindowManager().GetWindow(m_prevWindowID)};
        if (window && StringUtils::EqualsNoCase(URIUtils::GetFileName(window->GetProperty("xmlfile").asString()), info.GetData3()))
        {
          value = true;
          return true;
        }
      }
      break;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SYSTEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SYSTEM_HAS_ACTIVE_MODAL_DIALOG:
      value = CServiceBroker::GetGUI()->GetWindowManager().HasModalDialog(true);
      return true;
    case SYSTEM_HAS_VISIBLE_MODAL_DIALOG:
      value = CServiceBroker::GetGUI()->GetWindowManager().HasVisibleModalDialog();
      return true;
    case SYSTEM_HAS_INPUT_HIDDEN:
    {
      const CGUIDialogNumeric* pNumeric{
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogNumeric>(
              WINDOW_DIALOG_NUMERIC)};
      const CGUIDialogKeyboardGeneric* pKeyboard{
          CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogKeyboardGeneric>(
              WINDOW_DIALOG_KEYBOARD)};

      if (pNumeric && pNumeric->IsActive())
        value = pNumeric->IsInputHidden();
      else if (pKeyboard && pKeyboard->IsActive())
        value = pKeyboard->IsInputHidden();
      return true;
    }
    default:
      break;
  }

  return false;
}
