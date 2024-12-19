/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/guilib/VideoPlayActionProcessor.h"

namespace KODI::UTILS::GUILIB
{
class CGUIBuiltinsVideoPlayActionProcessor : public VIDEO::GUILIB::CVideoPlayActionProcessorBase
{
public:
  explicit CGUIBuiltinsVideoPlayActionProcessor(const ::std::shared_ptr<CFileItem>& item)
    : CVideoPlayActionProcessorBase(item)
  {
  }

protected:
  bool OnResumeSelected() override;
  bool OnPlaySelected() override;
};
} // namespace KODI::UTILS::GUILIB
