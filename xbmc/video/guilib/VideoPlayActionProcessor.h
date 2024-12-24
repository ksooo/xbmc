/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/guilib/VideoAction.h"

#include <memory>

class CFileItem;
class CFileItemList;

namespace KODI::VIDEO::GUILIB
{
class CVideoPlayActionProcessor
{
public:
  explicit CVideoPlayActionProcessor(const std::shared_ptr<CFileItem>& item) : m_item(item) {}
  virtual ~CVideoPlayActionProcessor() = default;

  bool ProcessDefaultAction();
  bool ProcessAction(Action action);

  void SetChoosePlayer() { m_choosePlayer = true; }
  void SetChooseStackPart() { m_chooseStackPart = true; }

  bool UserCancelled() const { return m_userCancelled; }

  static Action ChoosePlayOrResume(const CFileItem& item);

protected:
  virtual Action GetDefaultAction();
  virtual bool Process(Action action);

  virtual bool OnResumeSelected();
  virtual bool OnPlaySelected();

  void Play(const std::string& player);

  std::shared_ptr<CFileItem> m_item;
  bool m_userCancelled{false};
  bool m_choosePlayer{false};
  bool m_chooseStackPart{false};

private:
  CVideoPlayActionProcessor() = delete;
  unsigned int ChooseStackPart(CFileItemList& parts) const;
};
} // namespace KODI::VIDEO::GUILIB
