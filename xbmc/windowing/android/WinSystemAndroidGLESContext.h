/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "WinSystemAndroid.h"
#include "rendering/TopLayerGL.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "utils/EGLUtils.h"
#include "utils/GlobalsHandling.h"

struct ANativeWindow;
struct AVMasteringDisplayMetadata;
struct AVContentLightMetadata;

class CWinSystemAndroidGLESContext : public CWinSystemAndroid, public CRenderSystemGLES
{
public:
  CWinSystemAndroidGLESContext() = default;
  ~CWinSystemAndroidGLESContext() override = default;

  static void Register();
  static std::unique_ptr<CWinSystemBase> CreateWinSystem();

  // Implementation of CWinSystemBase via CWinSystemAndroid
  CRenderSystemBase *GetRenderSystem() override { return this; }
  bool InitWindowSystem() override;
  bool CreateNewWindow(const std::string& name,
                       bool fullScreen,
                       RESOLUTION_INFO& res) override;

  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  bool BeginTopLayer() override;
  void EndTopLayer() override;
  bool SuspendTopLayer() override;
  void ResumeTopLayer() override;

  /*!
   * \brief Render Kodi's GUI into a layer of its own instead of the main window.
   * \param window The window to present the layer on, nullptr to stop. Takes over the
   * caller's reference.
   */
  void SetTopLayerWindow(ANativeWindow* window);

  std::unique_ptr<CVideoSync> GetVideoSync(CVideoReferenceClock* clock) override;

  float GetFrameLatencyAdjustment() override;
  bool IsHDRDisplay() override;
  bool SetHDR(const VideoPicture* videoPicture) override;
  int GetOutputBitDepth() const override;
  KODI::UTILS::Eotf GetEotf() const override;
  KODI::UTILS::Colorimetry GetColorimetry() const override;

  EGLDisplay GetEGLDisplay() const;
  EGLSurface GetEGLSurface() const;
  EGLContext GetEGLContext() const;
  EGLConfig  GetEGLConfig() const;
protected:
  void SetVSyncImpl(bool enable) override;
  void PresentRenderImpl(bool rendered) override;

private:
  bool CreateSurface();
  void PresentTopLayer();

  CEGLContextUtils m_pGLContext;
  bool m_hasHDRConfig = false;

  std::unique_ptr<AVMasteringDisplayMetadata> m_displayMetadata;
  std::unique_ptr<AVContentLightMetadata> m_lightMetadata;
  EGLint m_HDRColorSpace = EGL_NONE;
  bool m_hasEGL_ST2086_Extension = false;
  bool m_hasEGL_BT2020_PQ_Colorspace_Extension = false;

  CCriticalSection m_topLayerLock;
  ANativeWindow* m_topLayerWindow{nullptr};
  EGLSurface m_topLayerSurface{EGL_NO_SURFACE};
  int m_topLayerSurfaceWidth{0};
  int m_topLayerSurfaceHeight{0};
  CTopLayerGL m_topLayer;
};
