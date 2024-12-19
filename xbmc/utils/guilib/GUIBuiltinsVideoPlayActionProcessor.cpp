/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIBuiltinsVideoPlayActionProcessor.h"

#include "utils/guilib/GUIBuiltinsUtils.h"

namespace KODI::UTILS::GUILIB
{
bool CGUIBuiltinsVideoPlayActionProcessor::OnResumeSelected()
{
  CGUIBuiltinsUtils::ExecutePlayMediaResume(m_item);
  return true;
}

bool CGUIBuiltinsVideoPlayActionProcessor::OnPlaySelected()
{
  CGUIBuiltinsUtils::ExecutePlayMediaNoResume(m_item);
  return true;
}
} // namespace KODI::UTILS::GUILIB
