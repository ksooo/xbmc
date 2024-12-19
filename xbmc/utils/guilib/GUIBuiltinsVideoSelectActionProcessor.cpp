/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIBuiltinsVideoSelectActionProcessor.h"

#include "utils/guilib/GUIBuiltinsUtils.h"

namespace KODI::UTILS::GUILIB
{
bool CGUIBuiltinsVideoSelectActionProcessor::OnPlayPartSelected(unsigned int part)
{
  CGUIBuiltinsUtils::ExecutePlayMediaPart(m_item, part);
  return true;
}

bool CGUIBuiltinsVideoSelectActionProcessor::OnResumeSelected()
{
  CGUIBuiltinsUtils::ExecutePlayMediaResume(m_item);
  return true;
}

bool CGUIBuiltinsVideoSelectActionProcessor::OnPlaySelected()
{
  CGUIBuiltinsUtils::ExecutePlayMediaNoResume(m_item);
  return true;
}

bool CGUIBuiltinsVideoSelectActionProcessor::OnQueueSelected()
{
  CGUIBuiltinsUtils::ExecuteQueueMedia(m_item);
  return true;
}
} // namespace KODI::UTILS::GUILIB
