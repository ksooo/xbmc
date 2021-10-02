/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "AndroidUtils.h"

#include "ServiceBroker.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingsManager.h"
#include "system_egl.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include "platform/android/activity/XBMCApp.h"

#include <cmath>
#include <cstdlib>

#include <androidjni/Build.h>
#include <androidjni/Display.h>
#include <androidjni/System.h>
#include <androidjni/SystemProperties.h>
#include <androidjni/View.h>
#include <androidjni/Window.h>
#include <androidjni/WindowManager.h>

namespace
{

bool s_hasModeApi = false;
std::vector<RESOLUTION_INFO> s_res_displayModes;
RESOLUTION_INFO s_res_cur_displayMode;

float currentRefreshRate()
{
  if (s_hasModeApi)
    return s_res_cur_displayMode.fRefreshRate;

  CJNIWindow window = CXBMCApp::getWindow();
  if (window)
  {
    const float preferredRate = window.getAttributes().getpreferredRefreshRate();
    if (preferredRate > 20.0 && preferredRate < 70.0)
    {
      CLog::Log(LOGINFO, "CAndroidUtils: Preferred refresh rate: %f", preferredRate);
      return preferredRate;
    }

    CJNIView view = window.getDecorView();
    if (view)
    {
      CJNIDisplay display = view.getDisplay();
      if (display)
      {
        const float reportedRate = display.getRefreshRate();
        if (reportedRate > 20.0 && reportedRate < 70.0)
        {
          CLog::Log(LOGINFO, "CAndroidUtils: Current display refresh rate: %f", reportedRate);
          return reportedRate;
        }
      }
    }
  }
  CLog::Log(LOGWARNING, "CAndroidUtils: Found no refresh rate, defaulting to 60.0Hz");
  return 60.0;
}

RESOLUTION_INFO getResolutionInfoFromDisplayMode(CJNIDisplayMode& mode)
{
  RESOLUTION_INFO info = {};

  info.strId = std::to_string(mode.getModeId());
  info.iWidth = mode.getPhysicalWidth();
  info.iScreenWidth = mode.getPhysicalWidth();
  info.iHeight = mode.getPhysicalHeight();
  info.iScreenHeight = mode.getPhysicalHeight();
  info.fRefreshRate = mode.getRefreshRate();
  info.dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  info.bFullScreen = true;
  info.iSubtitles = static_cast<int>(0.965 * info.iHeight);
  info.fPixelRatio = 1.0f;
  info.strMode =
      StringUtils::Format("%dx%d @ %.6f%s - Full Screen", info.iScreenWidth, info.iScreenHeight,
                          info.fRefreshRate, info.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  return info;
}

void fetchDisplayModes()
{
  s_hasModeApi = false;
  s_res_displayModes.clear();

  CJNIDisplay display = CXBMCApp::getWindow().getDecorView().getDisplay();

  if (display)
  {
    CJNIDisplayMode m = display.getMode();
    if (m)
    {
      if (m.getPhysicalWidth() > m.getPhysicalHeight())   // Assume unusable if portrait is returned
      {
        s_hasModeApi = true;

        CLog::Log(LOGDEBUG, "CAndroidUtils::fetchDisplayModes: current mode=%d, res=%dx%d@%f",
                  m.getModeId(), m.getPhysicalWidth(), m.getPhysicalHeight(), m.getRefreshRate());

        s_res_cur_displayMode = getResolutionInfoFromDisplayMode(m);

        const std::vector<CJNIDisplayMode> modes = display.getSupportedModes();
        for (auto mode : modes)
        {
          CLog::Log(LOGDEBUG, "CAndroidUtils::fetchDisplayModes: avail mode=%d, res=%dx%d@%f",
                    mode.getModeId(), mode.getPhysicalWidth(), mode.getPhysicalHeight(),
                    mode.getRefreshRate());

          RESOLUTION_INFO res = getResolutionInfoFromDisplayMode(mode);
          s_res_displayModes.emplace_back(res);
        }
      }
    }
  }
}

} // unnamed namespace

const std::string CAndroidUtils::SETTING_LIMITGUI = "videoscreen.limitgui";

CAndroidUtils::CAndroidUtils()
{
  UpdateDisplayModes();

  CServiceBroker::GetSettingsComponent()->GetSettings()->GetSettingsManager()->RegisterCallback(
      this, {CAndroidUtils::SETTING_LIMITGUI});
}

bool CAndroidUtils::GetNativeResolution(RESOLUTION_INFO& res) const
{
  const EGLNativeWindowType nativeWindow =
      static_cast<EGLNativeWindowType>(CXBMCApp::GetNativeWindow(30000));
  if (!nativeWindow)
    return false;

  if (!m_width || !m_height) // auto gui resolution setup?
  {
    ANativeWindow_acquire(nativeWindow);
    m_width = ANativeWindow_getWidth(nativeWindow);
    m_height= ANativeWindow_getHeight(nativeWindow);
    ANativeWindow_release(nativeWindow);
    CLog::Log(LOGINFO, "CAndroidUtils: native window resolution: %dx%d", m_width, m_height);
  }

  if (s_hasModeApi)
  {
    res = s_res_cur_displayMode;
    res.iWidth = m_width;
    res.iHeight = m_height;
  }
  else
  {
    res.strId = "-1";
    res.fRefreshRate = currentRefreshRate();
    res.dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
    res.bFullScreen = true;
    res.iWidth = m_width;
    res.iHeight = m_height;
    res.iScreenWidth = res.iWidth;
    res.iScreenHeight = res.iHeight;
    res.fPixelRatio = 1.0f;
  }

  res.iSubtitles = static_cast<int>(0.965 * res.iHeight);
  res.strMode =
      StringUtils::Format("%dx%d @ %.6f%s - Full Screen", res.iScreenWidth, res.iScreenHeight,
                          res.fRefreshRate, res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

  CLog::Log(LOGINFO, "CAndroidUtils::GetNativeResolution: mode=%s, res=%dx%d@%f (%dx%d)",
            res.strId.c_str(), res.iScreenWidth, res.iScreenHeight, res.fRefreshRate, res.iWidth,
            res.iHeight);
  return true;
}

bool CAndroidUtils::SetNativeResolution(const RESOLUTION_INFO& res)
{
  CLog::Log(LOGINFO, "CAndroidUtils::SetNativeResolution: mode=%s, res=%dx%d@%f (%dx%d)",
            res.strId.c_str(), res.iScreenWidth, res.iScreenHeight, res.fRefreshRate, res.iWidth,
            res.iHeight);

  if (s_hasModeApi)
  {
    UpdateDisplayModes();

    bool modeValid = false;
    for (const auto& mode : s_res_displayModes)
    {
      if (mode.strId == res.strId)
      {
        modeValid = true;
        break;
      }
    }

    if (modeValid)
    {
      CXBMCApp::SetDisplayMode(std::atoi(res.strId.c_str()), res.fRefreshRate);
      s_res_cur_displayMode = res;
    }
    else
    {
      CLog::Log(LOGERROR, "CAndroidUtils::SetNativeResolution: Invalid resolution!");
      return false;
    }
  }
  else
  {
    CXBMCApp::SetRefreshRate(res.fRefreshRate);
  }

  CXBMCApp::SetBuffersGeometry(res.iWidth, res.iHeight, 0);

  return true;
}

bool CAndroidUtils::ProbeResolutions(std::vector<RESOLUTION_INFO>& resolutions)
{
  RESOLUTION_INFO cur_res = {};
  bool ret = GetNativeResolution(cur_res);

  CLog::Log(LOGDEBUG, "CAndroidUtils: ProbeResolutions: %dx%d", m_width, m_height);

  if (s_hasModeApi)
  {
    for (RESOLUTION_INFO res : s_res_displayModes)
    {
      if (m_width && m_height)
      {
        res.iWidth = std::min(res.iWidth, m_width);
        res.iHeight = std::min(res.iHeight, m_height);
        res.iSubtitles = static_cast<int>(0.965 * res.iHeight);
      }
      resolutions.emplace_back(res);
    }
    return true;
  }

  if (ret && cur_res.iWidth > 1 && cur_res.iHeight > 1)
  {
    CJNIWindow window = CXBMCApp::getWindow();
    if (window)
    {
      std::vector<float> refreshRates;

      CJNIView view = window.getDecorView();
      if (view)
      {
        CJNIDisplay display = view.getDisplay();
        if (display)
        {
          refreshRates = display.getSupportedRefreshRates();
        }
      }

      for (const auto& rate : refreshRates)
      {
        if (rate < 20.0 || rate > 70.0)
          continue;

        cur_res.fRefreshRate = rate;
        cur_res.strMode = StringUtils::Format(
            "%dx%d @ %.6f%s - Full Screen", cur_res.iScreenWidth, cur_res.iScreenHeight,
            cur_res.fRefreshRate, cur_res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
        resolutions.emplace_back(cur_res);
      }
    }
    if (resolutions.empty())
    {
      /* No valid refresh rates available, just provide the current one */
      resolutions.emplace_back(cur_res);
    }
    return true;
  }
  return false;
}

bool CAndroidUtils::UpdateDisplayModes()
{
  m_width = 0;
  m_height = 0;

  if (CJNIBase::GetSDKVersion() >= 24)
  {
    fetchDisplayModes();
    for (const auto& res : s_res_displayModes)
    {
      // find max supported resolution
      if (res.iWidth > m_width || res.iHeight > m_height)
      {
        m_width = res.iWidth;
        m_height = res.iHeight;
      }
    }
  }

  if (!m_width || !m_height)
  {
    // Property available on some devices
    const std::string displaySize = CJNISystemProperties::get("sys.display-size", "");
    if (!displaySize.empty())
    {
      const std::vector<std::string> aSize = StringUtils::Split(displaySize, "x");
      if (aSize.size() == 2)
      {
        m_width = StringUtils::IsInteger(aSize[0]) ? std::atoi(aSize[0].c_str()) : 0;
        m_height = StringUtils::IsInteger(aSize[1]) ? std::atoi(aSize[1].c_str()) : 0;
      }
      CLog::Log(LOGDEBUG, "CAndroidUtils::UpdateDisplayModes: display-size: %s(%dx%d)",
                displaySize.c_str(), m_width, m_height);
    }
  }

  CLog::Log(LOGDEBUG, "CAndroidUtils::UpdateDisplayModes: maximum resolution: %dx%d", m_width,
            m_height);
  const int limit = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CAndroidUtils::SETTING_LIMITGUI);
  switch (limit)
  {
    case 0: // auto
      m_width = 0;
      m_height = 0;
      break;

    case 9999: // unlimited
      break;

    case 720:
      if (m_height > 720)
      {
        m_width = 1280;
        m_height = 720;
      }
      break;

    case 1080:
      if (m_height > 1080)
      {
        m_width = 1920;
        m_height = 1080;
      }
      break;
  }
  CLog::Log(LOGDEBUG, "CAndroidUtils: selected resolution: %dx%d", m_width, m_height);
  return true;
}

bool CAndroidUtils::IsHDRDisplay()
{
  bool ret = false;

  CJNIWindow window = CXBMCApp::getWindow();
  if (window)
  {
    CJNIView view = window.getDecorView();
    if (view)
    {
      CJNIDisplay display = view.getDisplay();
      if (display)
        ret = display.isHdr();
    }
  }
  CLog::Log(LOGDEBUG, "CAndroidUtils::IsHDRDisplay: %s", ret ? "true" : "false");
  return ret;
}

void CAndroidUtils::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  // Calibration (overscan / subtitles) are based on GUI size -> reset required
  if (setting->GetId() == CAndroidUtils::SETTING_LIMITGUI)
    CDisplaySettings::GetInstance().ClearCalibrations();
}
