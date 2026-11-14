/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemOSXGL.h"

#include "guilib/Texture.h"
#include "rendering/gl/RenderSystemGL.h"
#include "utils/log.h"
#include "windowing/WindowSystemFactory.h"

#include <mutex>

#import <Foundation/Foundation.h>
#import <OpenGL/CGLIOSurface.h>
#import <OpenGL/OpenGL.h>

void CWinSystemOSXGL::Register()
{
  KODI::WINDOWING::CWindowSystemFactory::RegisterWindowSystem(CreateWinSystem);
}

std::unique_ptr<CWinSystemBase> CWinSystemOSXGL::CreateWinSystem()
{
  return std::make_unique<CWinSystemOSXGL>();
}

void CWinSystemOSXGL::PresentRenderImpl(bool rendered)
{
  if (rendered)
  {
    PresentTopLayer();
    FlushBuffer();
  }

  if (m_delayDispReset && m_dispResetTimer.IsTimePast())
  {
    m_delayDispReset = false;
    AnnounceOnResetDevice();
  }
}

void CWinSystemOSXGL::SetVSyncImpl(bool enable)
{
  EnableVSync(false);

  if (enable)
  {
    EnableVSync(true);
  }
}

bool CWinSystemOSXGL::InitRenderSystem()
{
  if (!CRenderSystemGL::InitRenderSystem())
    return false;

  // Monitor synchronization may reset the video resolution, so the renderer must exist first.
  if (m_bWindowCreated)
    SynchronizeCurrentMonitor();

  return true;
}

bool CWinSystemOSXGL::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  CWinSystemOSX::ResizeWindow(newWidth, newHeight, newLeft, newTop);
  CRenderSystemGL::ResetRenderSystem(newWidth, newHeight);

  if (m_bVSync)
  {
    EnableVSync(m_bVSync);
  }

  return true;
}

bool CWinSystemOSXGL::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  CWinSystemOSX::SetFullScreen(fullScreen, res, blankOtherDisplays);
  CRenderSystemGL::ResetRenderSystem(res.iWidth - res.guiInsets.right - res.guiInsets.left,
                                     res.iHeight - res.guiInsets.top - res.guiInsets.bottom);

  if (m_bVSync)
  {
    EnableVSync(m_bVSync);
  }

  return true;
}

void CWinSystemOSXGL::SetTopLayerSink(TopLayerSink sink)
{
  std::unique_lock lock(m_topLayerLock);
  m_topLayerSink = std::move(sink);
}

bool CWinSystemOSXGL::BeginTopLayer()
{
  {
    std::unique_lock lock(m_topLayerLock);
    if (!m_topLayerSink)
      return false;
  }
  return m_topLayer.Begin(m_width, m_height, GetEnabledFrontToBackRendering());
}

void CWinSystemOSXGL::EndTopLayer()
{
  m_topLayer.End();
}

bool CWinSystemOSXGL::SuspendTopLayer()
{
  return m_topLayer.Suspend();
}

void CWinSystemOSXGL::ResumeTopLayer()
{
  m_topLayer.Resume();
}

bool CWinSystemOSXGL::PrepareTopLayerSurfaces()
{
  const int width = m_topLayer.GetWidth();
  const int height = m_topLayer.GetHeight();
  if (m_topLayerSurfaces[0].surface && m_topLayerSurfaceWidth == width &&
      m_topLayerSurfaceHeight == height)
    return true;

  ReleaseTopLayerSurfaces();
  m_topLayerSurfaceWidth = width;
  m_topLayerSurfaceHeight = height;

  NSDictionary* properties = @{
    (__bridge NSString*)kIOSurfaceWidth : @(width),
    (__bridge NSString*)kIOSurfaceHeight : @(height),
    (__bridge NSString*)kIOSurfaceBytesPerElement : @4,
    (__bridge NSString*)kIOSurfacePixelFormat : @(static_cast<uint32_t>('BGRA')),
  };
  bool complete = true;
  for (TopLayerSurface& target : m_topLayerSurfaces)
  {
    target.surface = IOSurfaceCreate((__bridge CFDictionaryRef)properties);
    if (!target.surface)
    {
      complete = false;
      break;
    }
    glGenTextures(1, &target.texture);
    glBindTexture(GL_TEXTURE_RECTANGLE, target.texture);
    CGLTexImageIOSurface2D(CGLGetCurrentContext(), GL_TEXTURE_RECTANGLE, GL_RGBA, width, height,
                           GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, target.surface, 0);
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    glGenFramebuffers(1, &target.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE,
                           target.texture, 0);
    complete = complete && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (!complete)
  {
    CLog::Log(LOGERROR, "CWinSystemOSXGL: cannot create the top layer surfaces");
    ReleaseTopLayerSurfaces();
  }
  return complete;
}

void CWinSystemOSXGL::ReleaseTopLayerSurfaces()
{
  for (TopLayerSurface& target : m_topLayerSurfaces)
  {
    glDeleteFramebuffers(1, &target.framebuffer);
    glDeleteTextures(1, &target.texture);
    if (target.surface)
      CFRelease(target.surface);
    target = {};
  }
}

void CWinSystemOSXGL::PresentTopLayer()
{
  TopLayerSink sink;
  {
    std::unique_lock lock(m_topLayerLock);
    sink = m_topLayerSink;
  }
  if (!sink)
  {
    if (m_topLayer.IsCreated())
    {
      m_topLayer.Release();
      ReleaseTopLayerSurfaces();
    }
    return;
  }
  if (!m_topLayer.TakeDirty() || !PrepareTopLayerSurfaces())
    return;

  // The layer is copied to two surfaces in turns, so the one on screen is not written to.
  const TopLayerSurface& target = m_topLayerSurfaces[m_topLayerSurfaceIndex];
  m_topLayerSurfaceIndex = (m_topLayerSurfaceIndex + 1) % m_topLayerSurfaces.size();

  const bool scissor = glIsEnabled(GL_SCISSOR_TEST);
  glDisable(GL_SCISSOR_TEST);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_topLayer.GetFramebuffer());
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.framebuffer);
  // Surface rows run top down.
  glBlitFramebuffer(0, 0, m_topLayerSurfaceWidth, m_topLayerSurfaceHeight, 0,
                    m_topLayerSurfaceHeight, m_topLayerSurfaceWidth, 0, GL_COLOR_BUFFER_BIT,
                    GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (scissor)
    glEnable(GL_SCISSOR_TEST);
  glFlush();

  sink(target.surface);
}
