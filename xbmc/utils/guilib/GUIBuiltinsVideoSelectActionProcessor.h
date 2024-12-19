/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/guilib/VideoSelectActionProcessor.h"

namespace KODI::UTILS::GUILIB
{
class CGUIBuiltinsVideoSelectActionProcessor : public VIDEO::GUILIB::CVideoSelectActionProcessorBase
{
public:
  CGUIBuiltinsVideoSelectActionProcessor(const std::shared_ptr<CFileItem>& item)
    : CVideoSelectActionProcessorBase(item)
  {
  }

protected:
  bool OnPlayPartSelected(unsigned int part) override;
  bool OnResumeSelected() override;
  bool OnPlaySelected() override;
  bool OnQueueSelected() override;
};
} // namespace KODI::UTILS::GUILIB
