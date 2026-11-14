/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "system_gl.h"

#if defined(HAS_GL) || HAS_GLES == 3

/*!
 * \brief Render target for the part of the GUI a platform shows in a layer above its own
 * windows on top of Kodi, see CWinSystemBase::BeginTopLayer.
 */
class CTopLayerGL
{
public:
  /*!
   * \brief Render to the layer, cleared, from now on.
   * \param clearDepth Whether the depth buffer is in use, as with front to back rendering
   * \return false if the layer cannot be created
   */
  bool Begin(int width, int height, bool clearDepth);
  void End();

  /*!
   * \brief Render to the window again in between.
   * \return true if rendering went to the layer, Resume must be called then
   */
  bool Suspend();
  void Resume();

  //! \return true once for every new image of the layer
  bool TakeDirty();

  bool IsCreated() const { return m_framebuffer != 0; }
  GLuint GetFramebuffer() const { return m_framebuffer; }
  int GetWidth() const { return m_width; }
  int GetHeight() const { return m_height; }
  void Release();

private:
  bool Create(int width, int height);

  GLuint m_framebuffer{0};
  GLuint m_texture{0};
  GLuint m_depthStencil{0};
  int m_width{0};
  int m_height{0};
  bool m_dirty{false};
  bool m_bound{false};
};

#endif
