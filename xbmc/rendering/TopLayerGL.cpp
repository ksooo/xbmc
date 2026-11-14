/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TopLayerGL.h"

#if defined(HAS_GL) || HAS_GLES == 3

#include "utils/log.h"

#include <utility>

bool CTopLayerGL::Begin(int width, int height, bool clearDepth)
{
  if ((m_framebuffer == 0 || m_width != width || m_height != height) && !Create(width, height))
    return false;

  // The scissor box of a partial update limits the clear to the area drawn anew.
  glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  GLbitfield flags = GL_COLOR_BUFFER_BIT;
  if (clearDepth)
  {
    glClearDepthf(0);
    glDepthMask(GL_TRUE);
    flags |= GL_DEPTH_BUFFER_BIT;
  }
  glClear(flags);
  m_bound = true;
  return true;
}

void CTopLayerGL::End()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  m_bound = false;
  m_dirty = true;
}

bool CTopLayerGL::Suspend()
{
  if (!m_bound)
    return false;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  m_bound = false;
  return true;
}

void CTopLayerGL::Resume()
{
  glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
  m_bound = true;
}

bool CTopLayerGL::TakeDirty()
{
  return std::exchange(m_dirty, false);
}

bool CTopLayerGL::Create(int width, int height)
{
  Release();
  m_width = width;
  m_height = height;

  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               nullptr);
  glBindTexture(GL_TEXTURE_2D, 0);
  glGenRenderbuffers(1, &m_depthStencil);
  glBindRenderbuffer(GL_RENDERBUFFER, m_depthStencil);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glGenFramebuffers(1, &m_framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                            m_depthStencil);
  const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

  // Partial updates only clear what they draw anew, so the new layer starts out transparent.
  const bool scissor = glIsEnabled(GL_SCISSOR_TEST);
  glDisable(GL_SCISSOR_TEST);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  if (scissor)
    glEnable(GL_SCISSOR_TEST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (!complete)
  {
    CLog::Log(LOGERROR, "CTopLayerGL: cannot create the framebuffer");
    Release();
  }
  return complete;
}

void CTopLayerGL::Release()
{
  glDeleteFramebuffers(1, &m_framebuffer);
  glDeleteTextures(1, &m_texture);
  glDeleteRenderbuffers(1, &m_depthStencil);
  m_framebuffer = 0;
  m_texture = 0;
  m_depthStencil = 0;
  m_dirty = false;
  m_bound = false;
}

#endif
