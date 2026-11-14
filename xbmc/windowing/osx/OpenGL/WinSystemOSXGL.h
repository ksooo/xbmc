/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "rendering/TopLayerGL.h"
#include "rendering/gl/RenderSystemGL.h"
#include "threads/CriticalSection.h"
#include "windowing/osx/WinSystemOSX.h"

#include <array>
#include <functional>

#include <IOSurface/IOSurface.h>

class CWinSystemOSXGL : public CWinSystemOSX, public CRenderSystemGL
{
public:
  CWinSystemOSXGL() = default;
  ~CWinSystemOSXGL() override = default;

  static void Register();
  static std::unique_ptr<CWinSystemBase> CreateWinSystem();

  // Implementation of CWinSystemBase via CWinSystemOSX
  CRenderSystemBase *GetRenderSystem() override { return this; }
  bool InitRenderSystem() override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  bool BeginTopLayer() override;
  void EndTopLayer() override;
  bool SuspendTopLayer() override;
  void ResumeTopLayer() override;

  using TopLayerSink = std::function<void(IOSurfaceRef)>;

  /*!
   * \brief Render Kodi's GUI into a layer of its own instead of the window.
   * \param sink Called on the render thread with each new image of the layer, empty to stop
   */
  void SetTopLayerSink(TopLayerSink sink);

protected:
  void PresentRenderImpl(bool rendered) override;
  void SetVSyncImpl(bool enable) override;

private:
  bool PrepareTopLayerSurfaces();
  void ReleaseTopLayerSurfaces();
  void PresentTopLayer();

  struct TopLayerSurface
  {
    IOSurfaceRef surface{nullptr};
    GLuint texture{0};
    GLuint framebuffer{0};
  };

  CCriticalSection m_topLayerLock;
  TopLayerSink m_topLayerSink;
  CTopLayerGL m_topLayer;
  std::array<TopLayerSurface, 2> m_topLayerSurfaces;
  size_t m_topLayerSurfaceIndex{0};
  int m_topLayerSurfaceWidth{0};
  int m_topLayerSurfaceHeight{0};
};
