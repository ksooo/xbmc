/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "controllers/ControllerTypes.h"

#include <future>
#include <memory>
#include <string>

class CInputManager;
class CProfileManager;

namespace ADDON
{
class CAddonMgr;
} // namespace ADDON

namespace PERIPHERALS
{
class CPeripherals;
}

namespace KODI
{
namespace RETRO
{
class CGUIGameRenderManager;
}

namespace SHADER
{
class CShaderPresetFactory;
}

namespace GAME
{
class CAgentInput;
class CControllerManager;
class CGameSettings;

/*!
 * \ingroup games
 */
class CGameServices
{
public:
  CGameServices(CControllerManager& controllerManager,
                RETRO::CGUIGameRenderManager& renderManager,
                PERIPHERALS::CPeripherals& peripheralManager,
                const CProfileManager& profileManager,
                CInputManager& inputManager,
                ADDON::CAddonMgr& addons);
  ~CGameServices();

  ControllerPtr GetController(const std::string& controllerId);
  ControllerPtr GetDefaultController();
  ControllerPtr GetDefaultKeyboard();
  ControllerPtr GetDefaultMouse();
  ControllerVector GetControllers();

  /*!
   * \brief Translate a feature on a controller into its localized name
   *
   * \param controllerId The controller ID that the feature belongs to
   * \param featureName The feature name
   *
   * \return The localized feature name, or empty if the controller or feature
   *         doesn't exist
   */
  std::string TranslateFeature(const std::string& controllerId, const std::string& featureName);

  std::string GetSavestatesFolder() const;

  CGameSettings& GameSettings() { return *m_gameSettings; }

  RETRO::CGUIGameRenderManager& GameRenderManager() { return m_gameRenderManager; }

  CAgentInput& AgentInput() { return *m_agentInput; }

  SHADER::CShaderPresetFactory& VideoShaders() { return *m_videoShaders; }

  /*!
   * \brief Called when an add-on repo is installed
   *
   * If the repo contains game add-ons, it can introduce new file extensions
   * to the list of known game extensions.
   */
  void OnAddonRepoInstalled();

private:
  // Construction parameters
  CControllerManager& m_controllerManager;
  RETRO::CGUIGameRenderManager& m_gameRenderManager;
  const CProfileManager& m_profileManager;

  // Game services
  std::unique_ptr<CGameSettings> m_gameSettings;
  std::unique_ptr<CAgentInput> m_agentInput;
  std::unique_ptr<SHADER::CShaderPresetFactory> m_videoShaders;

  // Game threads
  std::future<void> m_initializationTask;
};
} // namespace GAME
} // namespace KODI
