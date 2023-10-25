/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/guiinfo/GUIInfoProvider.h"

class CFavouritesGUIInfo : public KODI::GUILIB::GUIINFO::CGUIInfoProvider
{
public:
  CFavouritesGUIInfo();
  ~CFavouritesGUIInfo() override;

  // KODI::GUILIB::GUIINFO::IGUIInfoProvider implementation
  bool InitCurrentItem(CFileItem* item) override;
  bool GetLabel(std::string& value,
                const CFileItem* item,
                int contextWindow,
                const KODI::GUILIB::GUIINFO::CGUIInfo& info,
                std::string* fallback) const override;
  bool GetInt(int& value,
              const CGUIListItem* item,
              int contextWindow,
              const KODI::GUILIB::GUIINFO::CGUIInfo& info) const override;
  bool GetBool(bool& value,
               const CGUIListItem* item,
               int contextWindow,
               const KODI::GUILIB::GUIINFO::CGUIInfo& info) const override;
};
