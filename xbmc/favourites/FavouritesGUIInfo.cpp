/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FavouritesGUIInfo.h"

#include "FileItem.h"
#include "GUIInfoManager.h"
#include "ServiceBroker.h"
#include "favourites/FavouritesService.h"
#include "guilib/GUIComponent.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "music/tags/MusicInfoTag.h"
#include "video/VideoInfoTag.h"

using namespace KODI::GUILIB::GUIINFO;
using namespace MUSIC_INFO;

CFavouritesGUIInfo::CFavouritesGUIInfo()
{
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui)
    gui->GetInfoManager().RegisterInfoProvider(this);
}

CFavouritesGUIInfo::~CFavouritesGUIInfo()
{
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui)
    gui->GetInfoManager().UnregisterInfoProvider(this);
}

bool CFavouritesGUIInfo::InitCurrentItem(CFileItem* item)
{
  return false;
}

bool CFavouritesGUIInfo::GetLabel(std::string& value,
                                  const CFileItem* item,
                                  int contextWindow,
                                  const CGUIInfo& info,
                                  std::string* fallback) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // LISTITEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case LISTITEM_DBTYPE:
      if (item->IsFavourite())
      {
        const auto target{CServiceBroker::GetFavouritesService().ResolveFavourite(*item)};
        if (target)
        {
          if (target->HasVideoInfoTag())
          {
            const CVideoInfoTag* videoTag = target->GetVideoInfoTag();
            if (videoTag)
            {
              value = videoTag->m_type;
              return true;
            }
          }
          else if (target->HasMusicInfoTag())
          {
            const CMusicInfoTag* musicTag = target->GetMusicInfoTag();
            if (musicTag)
            {
              value = musicTag->GetType();
              return true;
            }
          }
        }
      }
      break;
  }

  return false;
}

bool CFavouritesGUIInfo::GetInt(int& value,
                                const CGUIListItem* gitem,
                                int contextWindow,
                                const CGUIInfo& info) const
{
  return false;
}

bool CFavouritesGUIInfo::GetBool(bool& value,
                                 const CGUIListItem* gitem,
                                 int contextWindow,
                                 const CGUIInfo& info) const
{
  return false;
}
