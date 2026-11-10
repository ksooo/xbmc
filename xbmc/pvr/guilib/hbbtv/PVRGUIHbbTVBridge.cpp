/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIHbbTVBridge.h"

#include "CompileInfo.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationVolumeHandling.h"
#include "guilib/GUIKeyboardFactory.h"
#include "messaging/ApplicationMessenger.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/guilib/PVRGUIActionsPlayback.h"
#include "pvr/hbbtv/PVRHbbTV.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <vector>

using namespace PVR;

namespace
{
// OIPF key set bits (hbbtv.objects.keyset).
constexpr int KEYSET_RED = 0x1;
constexpr int KEYSET_GREEN = 0x2;
constexpr int KEYSET_YELLOW = 0x4;
constexpr int KEYSET_BLUE = 0x8;
constexpr int KEYSET_NAVIGATION = 0x10;
constexpr int KEYSET_VCR = 0x20;
constexpr int KEYSET_NUMERIC = 0x100;
constexpr int KEYSET_MAXIMUM = 0x7FF;

// OIPF component types (hbbtv.objects.videobroadcast).
constexpr int COMPONENT_TYPE_VIDEO = 0;
constexpr int COMPONENT_TYPE_AUDIO = 1;
constexpr int COMPONENT_TYPE_SUBTITLE = 2;
constexpr int COMPONENT_TYPE_ALL = -1;
constexpr int ASPECT_RATIO_4_3 = 0;
constexpr int ASPECT_RATIO_16_9 = 1;

// Negative channel change result means success in the polyfill.
constexpr int CHANNEL_CHANGE_OK = -1;
constexpr int CHANNEL_STATUS_UNKNOWN_CHANNEL = 5;
// OIPF channel status reported through the ChannelStatusChanged event.
constexpr int CHANNEL_STATUS_PRESENTING = -3;

// HbbTV applications address the video in a 1280x720 plane.
constexpr float APP_PLANE_WIDTH = 1280.0f;
constexpr float APP_PLANE_HEIGHT = 720.0f;

int s_keySet = 0;

// Set while an application replaces itself by another one through createApplication.
bool s_handover = false;

// Set while Kodi's keyboard dialog collects text for the application.
bool s_textInputActive = false;

std::shared_ptr<IPVRHbbTVEngine> Engine()
{
  return CServiceBroker::GetPVRManager().Get<CPVRHbbTV>().GetEngine();
}

// Script that delivers a bridge event with the given properties to the polyfill.
std::string EventScript(const std::string& type, const CVariant& properties)
{
  std::string json;
  CJSONVariantWriter::Write(properties, json, true);
  return StringUtils::Format("document.dispatchBridgeEvent('{}',{})", type, json);
}

// Script that cuts a hole for the broadcast video out of the page, in 1280x720 plane
// coordinates. A zero size removes the hole. The clip sits on the body: WebKit paints
// some pages (RTL Digitaltext) black with a clip-path on the root element.
std::string VideoHoleScript(float x, float y, float width, float height)
{
  if (width <= 0.0f || height <= 0.0f)
    return "if(document.body)document.body.style.clipPath='';"
           "document.documentElement.style.backgroundColor=''";

  const long left = std::lround(x);
  const long top = std::lround(y);
  const long right = std::lround(x + width);
  const long bottom = std::lround(y + height);
  // A transparent root background would let the body background propagate to the canvas,
  // which the clip does not cover. A nearly invisible own background keeps it in the body.
  // Polygon coordinates are relative to the body box, which may be inset by its margin.
  return StringUtils::Format(
      "(function(){{var b=document.body;if(!b)return;var r=b.getBoundingClientRect();"
      "var l={}-r.left,t={}-r.top,rt={}-r.left,bt={}-r.top;"
      "b.style.clipPath='polygon(evenodd,-9999px -9999px,9999px -9999px,9999px 9999px,"
      "-9999px 9999px,-9999px -9999px,'+l+'px '+t+'px,'+l+'px '+bt+'px,'+rt+'px '+bt+'px,'"
      "+rt+'px '+t+'px,'+l+'px '+t+'px)';"
      "document.documentElement.style.backgroundColor='rgba(0,0,0,0.004)'}})()",
      left, top, right, bottom);
}

// Script that puts the text into the focused input field of the page.
std::string TextInputScript(const std::string& text)
{
  std::string quoted;
  CJSONVariantWriter::Write(CVariant{text}, quoted, true);
  return StringUtils::Format(
      "(function(){{var e=document.activeElement;if(!e)return;"
      "var t=e.tagName.toLowerCase();if(t==='input'||t==='textarea'){{e.value={};"
      "e.dispatchEvent(new Event('input'));e.dispatchEvent(new Event('change'));}}}})()",
      quoted);
}

// Tell the application that the broadcast of the playing channel is presented. The
// polyfill keeps the video/broadcast object in the connecting state until it hears this.
void NotifyBroadcastPresenting()
{
  const auto engine = Engine();
  const auto member =
      CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannelGroupMember();
  if (!engine || !member)
    return;

  CVariant event(CVariant::VariantTypeObject);
  event["onetId"] = 0;
  event["transId"] = 0;
  event["servId"] = member->Channel()->UniqueID();
  event["statusCode"] = CHANNEL_STATUS_PRESENTING;
  event["permanentError"] = false;
  engine->RunScript(EventScript("ChannelStatusChanged", event));
}

// Runs on the GUI thread: Kodi's keyboard dialog is modal there.
void ShowSoftKeyboard(void*)
{
  std::string text;
  const bool ok = CGUIKeyboardFactory::ShowAndGetInput(
      text, CVariant{CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(29998)},
      true);
  s_textInputActive = false;
  if (ok)
  {
    if (const auto engine = Engine())
      engine->RunScript(TextInputScript(text));
  }
}

KODI::MESSAGING::ThreadMessageCallback s_showSoftKeyboard{ShowSoftKeyboard, nullptr};

// Broadcast presentation requested by the application.
CRect s_videoWindow; // screen fractions, empty = full screen
bool s_presentationSuspended = false;

void ApplyBroadcastPresentation()
{
  // While suspended the broadcast is parked outside the screen and muted.
  const CRect window = s_presentationSuspended ? CRect(-1.0f, -1.0f, -0.5f, -0.5f) : s_videoWindow;
  CServiceBroker::GetWinSystem()->GetGfxContext().SetFullScreenVideoWindow(window);

  const auto appVolume =
      CServiceBroker::GetAppComponents().GetComponent<CApplicationVolumeHandling>();
  if (appVolume->IsMuted() != s_presentationSuspended)
    appVolume->SetMute(s_presentationSuspended);
}

// Resolves dvb://current.ait/<orgId>.<appId>[?query] against the signalled applications.
// Any other URL is returned unchanged. Empty if the application is not signalled.
std::string ResolveApplicationUrl(const std::string& url)
{
  static const std::string prefix = "dvb://current.ait/";
  if (!StringUtils::StartsWith(url, prefix))
    return url;

  std::string ids = url.substr(prefix.size());
  std::string query;
  const size_t queryPos = ids.find('?');
  if (queryPos != std::string::npos)
  {
    query = ids.substr(queryPos + 1);
    ids.resize(queryPos);
  }

  unsigned int orgId = 0;
  unsigned int appId = 0;
  if (std::sscanf(ids.c_str(), "%x.%x", &orgId, &appId) != 2)
    return {};

  for (const auto& app : CServiceBroker::GetPVRManager().Get<CPVRHbbTV>().GetApplications())
  {
    if (app.organisationId != orgId || app.applicationId != appId || app.url.empty())
      continue;
    if (query.empty())
      return app.url;
    return app.url + (app.url.find('?') == std::string::npos ? "?" : "&") + query;
  }
  return {};
}

CVariant EmptyArray()
{
  return CVariant(CVariant::VariantTypeArray);
}

CVariant ChannelToVariant(const std::shared_ptr<CPVRChannelGroupMember>& member)
{
  CVariant c(CVariant::VariantTypeObject);
  if (!member)
  {
    c["valid"] = false;
    return c;
  }

  const auto channel = member->Channel();
  c["valid"] = true;
  c["channelType"] = 0; // TV
  c["idType"] = 10; // DVB-C; Kodi does not know the delivery system
  c["ccid"] = StringUtils::Format("ccid:{}.{}", channel->ClientID(), channel->UniqueID());
  c["onid"] = 0;
  c["tsid"] = 0;
  c["sid"] = channel->UniqueID();
  c["name"] = channel->ChannelName();
  c["majorChannel"] = member->ChannelNumber().GetChannelNumber();
  c["hidden"] = channel->IsHidden();
  c["sourceID"] = 0;
  c["ipBroadcastID"] = "";
  return c;
}

CVariant ComponentToVariant(int type, int index, const StreamInfo& info, bool active)
{
  CVariant c(CVariant::VariantTypeObject);
  c["id"] = StringUtils::Format("{}:{}", type, index);
  c["type"] = type;
  c["componentTag"] = index;
  c["pid"] = 0;
  c["encoding"] = info.codecName;
  c["encrypted"] = false;
  c["active"] = active;
  c["hidden"] = false;
  c["language"] = info.language.AsIso6392T();
  return c;
}

// The streams of the playing channel as Kodi's player sees them. Applications read audio
// and subtitle tracks from here, e.g. to offer subtitles.
CVariant Components(int type)
{
  CVariant result(CVariant::VariantTypeArray);
  const auto player = CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>();

  if (type == COMPONENT_TYPE_ALL || type == COMPONENT_TYPE_VIDEO)
  {
    for (int i = 0; i < player->GetVideoStreamCount(); ++i)
    {
      VideoStreamInfo info;
      player->GetVideoStreamInfo(i, info);
      CVariant c =
          ComponentToVariant(COMPONENT_TYPE_VIDEO, i, info, i == player->GetVideoStream());
      c["aspectRatio"] = info.videoAspectRatio > 1.5f ? ASPECT_RATIO_16_9 : ASPECT_RATIO_4_3;
      result.push_back(c);
    }
  }
  if (type == COMPONENT_TYPE_ALL || type == COMPONENT_TYPE_AUDIO)
  {
    for (int i = 0; i < player->GetAudioStreamCount(); ++i)
    {
      AudioStreamInfo info;
      player->GetAudioStreamInfo(i, info);
      CVariant c =
          ComponentToVariant(COMPONENT_TYPE_AUDIO, i, info, i == player->GetAudioStream());
      c["audioDescription"] = (info.flags & StreamFlags::FLAG_VISUAL_IMPAIRED) != 0;
      c["audioChannels"] = info.channels;
      result.push_back(c);
    }
  }
  if (type == COMPONENT_TYPE_ALL || type == COMPONENT_TYPE_SUBTITLE)
  {
    const bool visible = player->GetSubtitleVisible();
    for (int i = 0; i < player->GetSubtitleCount(); ++i)
    {
      SubtitleStreamInfo info;
      player->GetSubtitleStreamInfo(i, info);
      CVariant c = ComponentToVariant(COMPONENT_TYPE_SUBTITLE, i, info,
                                      visible && i == player->GetSubtitle());
      c["hearingImpaired"] = (info.flags & StreamFlags::FLAG_HEARING_IMPAIRED) != 0;
      c["label"] = info.name;
      result.push_back(c);
    }
  }
  return result;
}

std::vector<std::shared_ptr<CPVRChannelGroupMember>> AllTVChannels()
{
  const auto group = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAllTV();
  if (!group)
    return {};
  return group->GetMembers(CPVRChannelGroup::Include::ONLY_VISIBLE);
}

std::shared_ptr<CPVRChannelGroupMember> FindByCcid(const std::string& ccid)
{
  int clientId = 0;
  int uid = 0;
  if (std::sscanf(ccid.c_str(), "ccid:%d.%d", &clientId, &uid) != 2)
    return {};

  for (const auto& member : AllTVChannels())
  {
    const auto channel = member->Channel();
    if (channel->ClientID() == clientId && channel->UniqueID() == uid)
      return member;
  }
  return {};
}

CVariant Capabilities()
{
  CVariant c(CVariant::VariantTypeObject);
  c["optionStrings"] = EmptyArray();
  c["optionStrings"].push_back("+DRM");
  c["profileNameFragments"] = EmptyArray();
  c["profileNameFragments"].push_back("+TRICKMODE");
  c["parentalSchemes"] = EmptyArray();
  c["graphicsLevels"] = EmptyArray();
  c["broadcastUrns"] = EmptyArray();
  c["displaySizeWidth"] = "";
  c["displaySizeHeight"] = "";
  c["displaySizeMeasurementType"] = "";
  c["audioOutputFormat"] = "";
  c["passThroughStatus"] = false;
  c["html5MediaVariableRateMin"] = "";
  c["html5MediaVariableRateMax"] = "";
  c["jsonRpcServerUrl"] = "";
  c["jsonRpcServerVersion"] = "";
  return c;
}

CVariant LocalSystem()
{
  CVariant c(CVariant::VariantTypeObject);
  c["vendorName"] = "Kodi";
  c["modelName"] = "Kodi";
  c["softwareVersion"] = "22.0";
  c["hardwareVersion"] = "1";
  c["serialNumber"] = "0";
  return c;
}

// Calls that always get the same answer. A null value means "void" (no result member).
const std::map<std::string, CVariant>& StaticResults()
{
  static const std::map<std::string, CVariant> results = {
      {"Broadcast.setChannelToTriplet", 0},
      {"Broadcast.setChannelToDsd", 0},
      {"Broadcast.setChannelToNull", CHANNEL_CHANGE_OK},
      {"Broadcast.getProgrammes", EmptyArray()},
      {"Broadcast.getPrivateAudioComponent", CVariant()},
      {"Broadcast.getPrivateVideoComponent", CVariant()},
      {"Broadcast.overrideComponentSelection", CVariant()},
      {"Broadcast.restoreComponentSelection", CVariant()},
      {"Broadcast.startSearch", false},
      {"Broadcast.abortSearch", false},
      {"Broadcast.addStreamEventListener", -1},
      {"Broadcast.removeStreamEventListener", CVariant()},
      {"Programme.getParentalRating", CVariant()},
      {"Programme.getSIDescriptors", EmptyArray()},
      {"Manager.getFreeMem", 256 * 1024 * 1024},
      {"Manager.getOtherKeyValues", EmptyArray()},
      {"Manager.getKeyMaximumValue", KEYSET_MAXIMUM},
      {"Manager.getKeyMaximumOtherKeys", 0},
      {"Manager.getKeyIcon", ""},
      {"Manager.getApplicationScheme", ""},
      {"Manager.getApplicationHowRelatedHref", ""},
      {"ParentalControl.getRatingSchemes", EmptyArray()},
      {"ParentalControl.getThreshold", CVariant()},
      {"ParentalControl.isRatingBlocked", false},
      {"ParentalControl.getPINLength", 4},
      {"ParentalControl.verifyPIN", false},
      {"ParentalControl.requestApproval", false},
      {"Configuration.getCapabilities", Capabilities()},
      {"Configuration.getAudioProfiles", EmptyArray()},
      {"Configuration.getVideoProfiles", EmptyArray()},
      {"Configuration.getVideoDisplayFormats", EmptyArray()},
      {"Configuration.getExtraSDVideoDecodes", 0},
      {"Configuration.getExtraHDVideoDecodes", 0},
      {"Configuration.getExtraUHDVideoDecodes", 0},
      {"Configuration.getBroadbandCapabilities", EmptyArray()},
      {"Configuration.getBroadcastCapabilities", EmptyArray()},
      {"Configuration.getLocalSystem", LocalSystem()},
      {"Configuration.getPreferredAudioLanguage", "deu"},
      {"Configuration.getPreferredSubtitleLanguage", "deu"},
      {"Configuration.getPreferredUILanguage", "deu"},
      {"Configuration.getCountryId", "DEU"},
      {"Configuration.getSubtitlesEnabled", false},
      {"Configuration.getAudioDescriptionEnabled", false},
      {"Configuration.getCleanAudioEnabled", false},
      {"Configuration.getDttNetworkIds", EmptyArray()},
      {"Configuration.getDeviceId", ""},
      {"Configuration.requestAccessToDistinctiveIdentifier", false},
      {"Drm.getSupportedDRMSystemIDs", EmptyArray()},
      {"Drm.sendDRMMessage", ""},
      {"Drm.canPlayContent", false},
      {"Drm.canRecordContent", false},
      {"Drm.setActiveDRM", false},
      {"Network.resolveHostAddress", ""},
  };
  return results;
}

// Returns true if the call produced a result (possibly null), false if it is void.
bool HandleDynamic(const std::string& method, const CVariant& params, CVariant& result)
{
  auto& pvr = CServiceBroker::GetPVRManager();

  if (method == "Broadcast.getCurrentChannel" || method == "Broadcast.getCurrentChannelForEvent")
  {
    result = ChannelToVariant(pvr.PlaybackState()->GetPlayingChannelGroupMember());
    return true;
  }
  if (method == "Broadcast.getChannelList")
  {
    result = EmptyArray();
    for (const auto& member : AllTVChannels())
      result.push_back(ChannelToVariant(member));
    return true;
  }
  if (method == "Broadcast.setChannelToCcid")
  {
    const auto member = FindByCcid(params["ccid"].asString());
    if (member && pvr.Get<PVR::GUI::Playback>().SwitchToChannel(CFileItem(member)))
      result = CHANNEL_CHANGE_OK;
    else
      result = CHANNEL_STATUS_UNKNOWN_CHANNEL;
    return true;
  }
  if (method == "Broadcast.getComponents")
  {
    result = Components(static_cast<int>(params["type"].asInteger(COMPONENT_TYPE_ALL)));
    return true;
  }
  if (method == "Broadcast.setVideoRectangle")
  {
    const float x = params["x"].asFloat();
    const float y = params["y"].asFloat();
    const float width = params["width"].asFloat();
    const float height = params["height"].asFloat();
    CLog::Log(LOGINFO, "HbbTV bridge: video rectangle {}x{}+{}+{}", width, height, x, y);

    const auto engine = Engine();
    if (!engine)
      return false;

    s_videoWindow = CRect();
    const bool fullScreen =
        x == 0.0f && y == 0.0f && width == APP_PLANE_WIDTH && height == APP_PLANE_HEIGHT;
    if (!fullScreen)
    {
      // The application plane is the largest centred 16:9 area of the window, see the view,
      // and Kodi's full screen video window covers the window.
      float windowWidth;
      float windowHeight;
      if (!engine->GetContentSize(windowWidth, windowHeight))
        return false;
      const float scale = std::min(windowWidth / APP_PLANE_WIDTH, windowHeight / APP_PLANE_HEIGHT);
      const float offsetX = (windowWidth - APP_PLANE_WIDTH * scale) / 2;
      const float offsetY = (windowHeight - APP_PLANE_HEIGHT * scale) / 2;
      s_videoWindow =
          CRect((offsetX + x * scale) / windowWidth, (offsetY + y * scale) / windowHeight,
                (offsetX + (x + width) * scale) / windowWidth,
                (offsetY + (y + height) * scale) / windowHeight);
    }
    ApplyBroadcastPresentation();

    // A scaled video sits behind page content; cut it free. Full screen video is overlaid
    // by the page as a whole, so no hole is needed there.
    if (fullScreen || x < 0.0f || y < 0.0f)
      engine->RunScript(VideoHoleScript(0.0f, 0.0f, 0.0f, 0.0f));
    else
      engine->RunScript(VideoHoleScript(x, y, width, height));
    return false;
  }
  if (method == "Broadcast.setPresentationSuspended")
  {
    s_presentationSuspended = params["presentationSuspended"].asBoolean();
    CLog::Log(LOGINFO, "HbbTV bridge: broadcast presentation suspended: {}",
              s_presentationSuspended);
    ApplyBroadcastPresentation();
    if (!s_presentationSuspended)
      NotifyBroadcastPresenting();
    return false;
  }
  if (method == "Broadcast.getVolume" || method == "Broadcast.setVolume")
  {
    const auto appVolume =
        CServiceBroker::GetAppComponents().GetComponent<CApplicationVolumeHandling>();
    if (method == "Broadcast.setVolume")
    {
      appVolume->SetVolume(static_cast<float>(params["volume"].asInteger()));
      result = true;
    }
    else
      result = static_cast<int>(appVolume->GetVolumePercent());
    return true;
  }
  if (method == "Manager.createApplication")
  {
    const std::string url = ResolveApplicationUrl(params["url"].asString());
    if (url.empty())
    {
      CLog::Log(LOGWARNING, "HbbTV bridge: cannot resolve application '{}'",
                params["url"].asString());
      result = false;
      return true;
    }
    s_handover = true;
    if (const auto engine = Engine())
      engine->LoadUrl(url);
    result = true;
    return true;
  }
  if (method == "Manager.destroyApplication")
  {
    if (s_handover)
    {
      // The application replaced itself by the one just created and now quits.
      CLog::Log(LOGDEBUG, "HbbTV bridge: application handed over, keeping the view");
      s_handover = false;
      return false;
    }
    // The application quits, like after the exit key.
    CServiceBroker::GetPVRManager().Get<CPVRHbbTV>().CloseApplication();
    return false;
  }
  if (method == "Manager.showApplication" || method == "Manager.hideApplication")
  {
    if (const auto engine = Engine())
      engine->SetVisible(method == "Manager.showApplication");
    return false;
  }
  if (method == "Manager.getKeyValues")
  {
    result = s_keySet;
    return true;
  }
  if (method == "Manager.setKeyValue")
  {
    // Applications pass undefined at times (RTL launch bar). Keep the current set then,
    // as TV sets do.
    if (params["value"].isInteger() || params["value"].isDouble())
    {
      s_keySet = static_cast<int>(params["value"].asInteger()) & KEYSET_MAXIMUM;
      CLog::Log(LOGDEBUG, "HbbTV bridge: key set {:#x}", s_keySet);
    }
    result = s_keySet;
    return true;
  }
  if (method == "SoftKeyboard.show")
  {
    // The request arrives on the browser's thread; the dialog must run on Kodi's GUI thread.
    if (!s_textInputActive)
    {
      s_textInputActive = true;
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_CALLBACK, -1, -1,
                                                 static_cast<void*>(&s_showSoftKeyboard));
    }
    return false;
  }

  const auto& statics = StaticResults();
  const auto it = statics.find(method);
  if (it == statics.end())
  {
    CLog::Log(LOGWARNING, "HbbTV bridge: unhandled method '{}'", method);
    return false;
  }
  result = it->second;
  return !result.isNull();
}
} // unnamed namespace

std::string CPVRGUIHbbTVBridge::HandleRequest(const std::string& json)
{
  CVariant request;
  CVariant response(CVariant::VariantTypeObject);

  if (!CJSONVariantParser::Parse(json, request) || !request.isMember("method"))
  {
    CLog::Log(LOGERROR, "HbbTV bridge: invalid request '{}'", json);
    response["error"] = "InvalidRequest";
  }
  else
  {
    const std::string method = request["method"].asString();
    CLog::Log(LOGDEBUG, "HbbTV bridge: {}", method);

    CVariant result;
    if (HandleDynamic(method, request["params"], result))
      response["result"] = result;
  }

  std::string output;
  CJSONVariantWriter::Write(response, output, true);
  return output;
}

bool CPVRGUIHbbTVBridge::AcceptsKey(int virtualKey)
{
  int keySet = 0;
  switch (virtualKey)
  {
    case 403: // VK_RED
      keySet = KEYSET_RED;
      break;
    case 404: // VK_GREEN
      keySet = KEYSET_GREEN;
      break;
    case 405: // VK_YELLOW
      keySet = KEYSET_YELLOW;
      break;
    case 406: // VK_BLUE
      keySet = KEYSET_BLUE;
      break;
    case 13: // VK_ENTER
    case 37: // VK_LEFT
    case 38: // VK_UP
    case 39: // VK_RIGHT
    case 40: // VK_DOWN
    case 461: // VK_BACK
      keySet = KEYSET_NAVIGATION;
      break;
    case 19: // VK_PAUSE
    case 402: // VK_PLAY_PAUSE
    case 412: // VK_REWIND
    case 413: // VK_STOP
    case 415: // VK_PLAY
    case 417: // VK_FAST_FWD
      keySet = KEYSET_VCR;
      break;
    default:
      if (virtualKey >= '0' && virtualKey <= '9')
        keySet = KEYSET_NUMERIC;
      break;
  }
  return (s_keySet & keySet) != 0;
}

bool CPVRGUIHbbTVBridge::IsTextInputActive()
{
  return s_textInputActive;
}

void CPVRGUIHbbTVBridge::OnNewDocument()
{
  s_handover = false;
  s_keySet = 0;
  ResetBroadcastPresentation();
}

void CPVRGUIHbbTVBridge::ResetBroadcastPresentation()
{
  s_videoWindow = CRect();
  s_presentationSuspended = false;
  ApplyBroadcastPresentation();
}

std::string CPVRGUIHbbTVBridge::KeyEventScript(int virtualKey, const std::string& type)
{
  // Constructed KeyboardEvents ignore keyCode, so define it. The polyfill's KeyEvent
  // handler translates events without a 'code' into the proper HbbTV key.
  return StringUtils::Format(
      "(function(){{var t=document.activeElement||document;"
      "var e=new KeyboardEvent('{}',{{bubbles:true,cancelable:true}});"
      "Object.defineProperty(e,'keyCode',{{value:{}}});"
      "Object.defineProperty(e,'which',{{value:{}}});"
      "t.dispatchEvent(e);}})()",
      type, virtualKey, virtualKey);
}

std::string CPVRGUIHbbTVBridge::UserAgent(const std::string& engine)
{
  // HbbTV token: (capabilities;vendor;model;software version;hardware version;family)
  return StringUtils::Format("HbbTV/1.6.1 (+DRM;{};{};{};;) Mozilla/5.0 (Linux; SmartTV) {}",
                             CCompileInfo::GetAppName(), CSysInfo::GetOsName(),
                             CSysInfo::GetVersionShort(), engine);
}

void CPVRGUIHbbTVBridge::LogConsole(const std::string& message, bool error)
{
  if (error)
    CLog::Log(LOGWARNING, "HbbTV console: {}", message);
  else
    CLog::LogFC(LOGDEBUG, LOGPVR, "HbbTV console: {}", message);
}
