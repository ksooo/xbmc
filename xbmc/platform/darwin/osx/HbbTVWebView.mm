/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "HbbTVWebView.h"

#include "ServiceBroker.h"
#include "input/keyboard/KeyboardTypes.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/hbbtv/PVRGUIHbbTVBridge.h"
#include "pvr/guilib/hbbtv/PVRGUIHbbTVKeyRouter.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"
#include "windowing/osx/OpenGL/WinSystemOSXGL.h"
#include "windowing/osx/WinSystemOSX.h"

#include <algorithm>
#include <cmath>

#import <AppKit/AppKit.h>
// WebKit needs the Network framework types. <Network/Network.h> would resolve to
// xbmc/network/Network.h on a case-insensitive filesystem, so pull in a sub-header directly.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-W#warnings"
#import <Network/proxy_config.h>
#pragma clang diagnostic pop
#import <WebKit/WebKit.h>

using PVR::CPVRGUIHbbTVBridge;
using PVR::CPVRGUIHbbTVKeyRouter;

namespace
{
constexpr const char* ORB_SCHEME = "orb";
constexpr const char* BRIDGE_PROMPT = "__orb_bridge__";
constexpr const char* ENGINE_PRODUCT = "AppleWebKit/605.1.15 (KHTML, like Gecko)";

// HbbTV applications are authored for a 1280x720 graphics plane.
constexpr CGFloat APP_PLANE_WIDTH = 1280.0;
constexpr CGFloat APP_PLANE_HEIGHT = 720.0;

// Routes console output and uncaught errors of the page into the Kodi log.
NSString* const CONSOLE_SCRIPT =
    @"(function(){"
     "const post=function(e,t){try{window.webkit.messageHandlers.kodiLog.postMessage({error:e,"
     "text:t})}catch(x){}};"
     "const send=function(l){return function(){var a=[];for(var i=0;i<arguments.length;i++){var "
     "x=arguments[i];try{a.push(typeof x==='object'?JSON.stringify(x):String(x))}catch(e){a.push("
     "String(x))}}"
     "post(l==='error',l+': '+a.join(' '))}};"
     "console.log=send('log');console.debug=send('debug');console.info=send('info');"
     "console.warn=send('warn');console.error=send('error');"
     "window.addEventListener('error',function(e){post(true,'uncaught: '+e.message+' @ '+"
     "e.filename+':'+e.lineno+(e.error&&e.error.stack?'\\n'+e.error.stack:''))});"
     "window.addEventListener('unhandledrejection',function(e){var r=e.reason;post(true,"
     "'unhandled rejection: '+(r&&r.stack?r.stack:String(r)))});"
     "})();";

// A TV set shows no scroll bars. With a mouse connected macOS draws them permanently, in
// a track of their own that takes space away from the page.
NSString* const HIDE_SCROLL_BARS_SCRIPT =
    @"(function(){"
     "const style=document.createElement('style');"
     "style.textContent='::-webkit-scrollbar{display:none}';"
     "document.documentElement.appendChild(style);"
     "})();";

NSString* UserAgent()
{
  return @(CPVRGUIHbbTVBridge::UserAgent(ENGINE_PRODUCT).c_str());
}

void RunOnMainThread(void (^block)())
{
  if (NSThread.isMainThread)
    block();
  else
    dispatch_sync(dispatch_get_main_queue(), block);
}

// All views share one process pool, so a warmed up web content process serves the first
// application as well.
WKProcessPool* ProcessPool()
{
  static WKProcessPool* pool = [WKProcessPool new];
  return pool;
}
} // unnamed namespace

/*!
 * Serves the polyfill's internal player page (orb://player). The polyfill loads broadband
 * media in an iframe with this page, which hosts the actual video element.
 */
@interface KodiOrbSchemeHandler : NSObject <WKURLSchemeHandler>
@end

@implementation KodiOrbSchemeHandler

- (void)webView:(WKWebView*)webView startURLSchemeTask:(id<WKURLSchemeTask>)task
{
  NSURL* url = task.request.URL;
  if (![url.host isEqualToString:@"player"])
  {
    [task didFailWithError:[NSError errorWithDomain:NSURLErrorDomain
                                               code:NSURLErrorFileDoesNotExist
                                           userInfo:nil]];
    return;
  }

  const std::string page = CPVRGUIHbbTVBridge::PlayerPage(url.absoluteString.UTF8String);
  if (page.empty())
  {
    [task didFailWithError:[NSError errorWithDomain:NSURLErrorDomain
                                               code:NSURLErrorResourceUnavailable
                                           userInfo:nil]];
    return;
  }
  NSData* body = [NSData dataWithBytes:page.data() length:page.size()];

  NSHTTPURLResponse* response =
      [[NSHTTPURLResponse alloc] initWithURL:url
                                  statusCode:200
                                 HTTPVersion:@"HTTP/1.1"
                                headerFields:@{@"Content-Type" : @"text/html; charset=utf-8"}];
  [task didReceiveResponse:response];
  [task didReceiveData:body];
  [task didFinish];
}

- (void)webView:(WKWebView*)webView stopURLSchemeTask:(id<WKURLSchemeTask>)task
{
}

@end

/*!
 * Overlay window. It is the key window while an application runs so that the page is
 * active and shows its focus. Keys the application does not take go on to Kodi's window.
 */
@interface KodiHbbTVWindow : NSWindow
@end

@implementation KodiHbbTVWindow

- (BOOL)canBecomeKeyWindow
{
  return YES;
}

- (void)becomeKeyWindow
{
  [super becomeKeyWindow];
  // Kodi stops processing input when its own window resigns key. Keep it going.
  if (auto* winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem()))
    winSystem->enableInputEvents();
}

// AppKit offers function keys such as the arrows to the views as key equivalents first, and
// the web view takes them. All keys shall go through sendEvent, where the application only
// gets the keys it asked for.
- (BOOL)performKeyEquivalent:(NSEvent*)event
{
  return NO;
}

- (void)sendEvent:(NSEvent*)event
{
  if (event.type == NSEventTypeKeyDown || event.type == NSEventTypeKeyUp)
  {
    if (!CHbbTVWebView::HandleKeyEvent(event))
      [self.parentWindow sendEvent:event];
    return;
  }
  [super sendEvent:event];
}

@end

@interface KodiHbbTVView : NSObject <WKUIDelegate, WKNavigationDelegate, WKScriptMessageHandler>
@property(nonatomic, strong) NSWindow* parent;
@property(nonatomic, strong) NSWindow* window;
@property(nonatomic, strong) WKWebView* webView;
@property(nonatomic, strong) NSView* topLayerView;
@property(nonatomic) NSSize contentSize;
@end

@implementation KodiHbbTVView

- (instancetype)initWithParent:(NSWindow*)parent url:(NSString*)url
{
  self = [super init];
  if (!self)
    return nil;

  _parent = parent;

  WKWebViewConfiguration* config = [WKWebViewConfiguration new];
  config.processPool = ProcessPool();
  config.mediaTypesRequiringUserActionForPlayback = WKAudiovisualMediaTypeNone;
  // HbbTV applications are served over http. Upgraded hosts like hbbtv.rtl2.de answer https
  // with a foreign certificate, and the navigation fails.
  if (@available(macOS 11.3, *))
    config.upgradeKnownHostsToHTTPS = NO;
  [config setURLSchemeHandler:[KodiOrbSchemeHandler new] forURLScheme:@(ORB_SCHEME)];

  WKUserContentController* content = [WKUserContentController new];
  [content addScriptMessageHandler:self name:@"kodiLog"];
  [content
      addUserScript:[[WKUserScript alloc] initWithSource:CONSOLE_SCRIPT
                                           injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                        forMainFrameOnly:NO]];
  [content
      addUserScript:[[WKUserScript alloc] initWithSource:HIDE_SCROLL_BARS_SCRIPT
                                           injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                        forMainFrameOnly:NO]];

  const std::string script = CPVRGUIHbbTVBridge::PolyfillScript(url.UTF8String);
  NSString* polyfill = [[NSString alloc] initWithBytes:script.data()
                                                length:script.size()
                                              encoding:NSUTF8StringEncoding];
  if (polyfill.length > 0)
    [content
        addUserScript:[[WKUserScript alloc] initWithSource:polyfill
                                             injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                          forMainFrameOnly:YES]];
  config.userContentController = content;

  NSRect contentRect = [parent contentRectForFrameRect:parent.frame];

  _webView = [[WKWebView alloc]
      initWithFrame:NSMakeRect(0, 0, contentRect.size.width, contentRect.size.height)
      configuration:config];
  _webView.customUserAgent = UserAgent();
  _webView.UIDelegate = self;
  _webView.navigationDelegate = self;
  [_webView setValue:@NO forKey:@"drawsBackground"];

  NSView* container = [[NSView alloc] initWithFrame:_webView.frame];
  [container addSubview:_webView];

  // Kodi renders its GUI into this view's layer, shown above the application.
  _topLayerView = [[NSView alloc] initWithFrame:container.bounds];
  _topLayerView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  _topLayerView.wantsLayer = YES;
  [container addSubview:_topLayerView];
  if (auto* winSystem = dynamic_cast<CWinSystemOSXGL*>(CServiceBroker::GetWinSystem()))
  {
    __weak NSView* topLayerView = _topLayerView;
    winSystem->SetTopLayerSink([topLayerView](IOSurfaceRef surface) {
      id contents = (__bridge id)surface;
      dispatch_async(dispatch_get_main_queue(), ^{
        topLayerView.layer.contents = contents;
      });
    });
  }

  _window = [[KodiHbbTVWindow alloc] initWithContentRect:contentRect
                                               styleMask:NSWindowStyleMaskBorderless
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];
  _window.opaque = NO;
  _window.backgroundColor = NSColor.clearColor;
  _window.hasShadow = NO;
  _window.ignoresMouseEvents = YES;
  _window.releasedWhenClosed = NO;
  _window.contentView = container;
  [parent addChildWindow:_window ordered:NSWindowAbove];
  [_window makeKeyWindow];
  [_window makeFirstResponder:_webView];

  [NSNotificationCenter.defaultCenter addObserver:self
                                         selector:@selector(parentDidResize:)
                                             name:NSWindowDidResizeNotification
                                           object:parent];
  [self updateGeometry];

  [self loadUrl:url];
  return self;
}

- (void)dealloc
{
  [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)parentDidResize:(NSNotification*)notification
{
  [self updateGeometry];
}

- (void)updateGeometry
{
  // The window keeps its titled style mask in native full screen, so derive the drawable
  // area from the content view itself rather than from the frame.
  NSView* contentView = self.parent.contentView;
  NSRect contentRect =
      [self.parent convertRectToScreen:[contentView convertRect:contentView.bounds toView:nil]];
  self.contentSize = [contentView convertRectToBacking:contentView.bounds].size;
  // The 1280x720 application plane is the largest 16:9 area centred in the window, like a
  // full screen video. Kodi's bridge maps video rectangles with the same geometry.
  const CGFloat scale = std::min(contentRect.size.width / APP_PLANE_WIDTH,
                                 contentRect.size.height / APP_PLANE_HEIGHT);
  NSRect planeRect;
  planeRect.size = NSMakeSize(APP_PLANE_WIDTH * scale, APP_PLANE_HEIGHT * scale);
  planeRect.origin = NSMakePoint((contentRect.size.width - planeRect.size.width) / 2,
                                 (contentRect.size.height - planeRect.size.height) / 2);
  [self.window setFrame:contentRect display:YES];
  self.webView.frame = planeRect;
  if (@available(macOS 11.0, *))
    self.webView.pageZoom = scale;
}

- (void)loadUrl:(NSString*)url
{
  CLog::Log(LOGINFO, "HbbTV: loading '{}'", url.UTF8String);
  NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:url]];
  [request setValue:@(CPVRGUIHbbTVBridge::ACCEPT_HEADER) forHTTPHeaderField:@"Accept"];
  [self.webView loadRequest:request];
}

- (void)close
{
  if (auto* winSystem = dynamic_cast<CWinSystemOSXGL*>(CServiceBroker::GetWinSystem()))
    winSystem->SetTopLayerSink({});
  [self.parent removeChildWindow:self.window];
  [self.window orderOut:nil];
  [self.parent makeKeyWindow];
  [self.webView stopLoading];
  [self.webView.configuration.userContentController removeScriptMessageHandlerForName:@"kodiLog"];
  self.webView.UIDelegate = nil;
  self.webView.navigationDelegate = nil;
}

- (void)dispatchKey:(int)virtualKey type:(NSString*)type
{
  [self runScript:@(CPVRGUIHbbTVBridge::KeyEventScript(virtualKey, type.UTF8String).c_str())];
}

- (void)forwardKeyEvent:(NSEvent*)event
{
  // A real event is trusted by the page, so Enter activates the focused element and
  // consent dialogs accept the input.
  if (event.type == NSEventTypeKeyDown)
    [self.webView keyDown:event];
  else
    [self.webView keyUp:event];
}

- (void)runScript:(NSString*)script
{
  [self.webView evaluateJavaScript:script completionHandler:nil];
}

#pragma mark - WKUIDelegate

- (void)webView:(WKWebView*)webView
    runJavaScriptTextInputPanelWithPrompt:(NSString*)prompt
                              defaultText:(NSString*)defaultText
                         initiatedByFrame:(WKFrameInfo*)frame
                        completionHandler:(void (^)(NSString*))completionHandler
{
  if (![prompt isEqualToString:@(BRIDGE_PROMPT)])
  {
    completionHandler(nil);
    return;
  }

  const std::string response = CPVRGUIHbbTVBridge::HandleRequest(defaultText.UTF8String);
  completionHandler(@(response.c_str()));
}

- (void)webView:(WKWebView*)webView
    runJavaScriptAlertPanelWithMessage:(NSString*)message
                      initiatedByFrame:(WKFrameInfo*)frame
                     completionHandler:(void (^)(void))completionHandler
{
  CLog::Log(LOGINFO, "HbbTV alert: {}", message.UTF8String);
  completionHandler();
}

#pragma mark - WKNavigationDelegate

- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationResponse:(WKNavigationResponse*)navigationResponse
                      decisionHandler:(void (^)(WKNavigationResponsePolicy))decisionHandler
{
  if (navigationResponse.canShowMIMEType || !navigationResponse.forMainFrame)
  {
    decisionHandler(WKNavigationResponsePolicyAllow);
    return;
  }

  // WebKit refuses application/vnd.hbbtv.xhtml+xml. Fetch the document ourselves and
  // hand it over as XHTML.
  NSURLResponse* response = navigationResponse.response;
  CLog::Log(LOGINFO, "HbbTV: re-fetching '{}' with unsupported MIME type '{}'",
            response.URL.absoluteString.UTF8String, response.MIMEType.UTF8String);
  decisionHandler(WKNavigationResponsePolicyCancel);

  // Kodi's own HTTP client is not subject to App Transport Security, which the app only relaxes
  // for web content. The fetch shares the WebView's cookies in both directions.
  NSURL* url = response.URL;
  WKHTTPCookieStore* cookieStore = webView.configuration.websiteDataStore.httpCookieStore;
  __weak KodiHbbTVView* weakSelf = self;
  [cookieStore getAllCookies:^(NSArray<NSHTTPCookie*>* cookies) {
    NSHTTPCookieStorage* jar =
        NSURLSessionConfiguration.ephemeralSessionConfiguration.HTTPCookieStorage;
    for (NSHTTPCookie* cookie in cookies)
      [jar setCookie:cookie];
    NSString* cookieHeader =
        [NSHTTPCookie requestHeaderFieldsWithCookies:[jar cookiesForURL:url]][@"Cookie"];

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
      CPVRGUIHbbTVBridge::Document document;
      const bool fetched =
          CPVRGUIHbbTVBridge::FetchDocument(url.absoluteString.UTF8String, ENGINE_PRODUCT,
                                            cookieHeader ? cookieHeader.UTF8String : "", document);
      NSURL* finalUrl = [NSURL URLWithString:@(document.url.c_str())] ?: url;
      NSMutableArray<NSHTTPCookie*>* received = [NSMutableArray array];
      for (const std::string& value : document.cookies)
        [received addObjectsFromArray:[NSHTTPCookie
                                          cookiesWithResponseHeaderFields:@{
                                            @"Set-Cookie" : @(value.c_str())
                                          }
                                                                   forURL:finalUrl]];
      NSData* data = [NSData dataWithBytes:document.body.data() length:document.body.size()];

      dispatch_async(dispatch_get_main_queue(), ^{
        KodiHbbTVView* strongSelf = weakSelf;
        if (!strongSelf || !fetched)
          return;
        dispatch_group_t stored = dispatch_group_create();
        for (NSHTTPCookie* cookie in received)
        {
          dispatch_group_enter(stored);
          [cookieStore setCookie:cookie
               completionHandler:^{
                 dispatch_group_leave(stored);
               }];
        }
        dispatch_group_notify(stored, dispatch_get_main_queue(), ^{
          [strongSelf.webView loadData:data
                              MIMEType:@"application/xhtml+xml"
                 characterEncodingName:@"UTF-8"
                               baseURL:finalUrl];
        });
      });
    });
  }];
}

- (void)webView:(WKWebView*)webView
    didFailProvisionalNavigation:(WKNavigation*)navigation
                       withError:(NSError*)error
{
  // A superseded navigation reports NSURLErrorCancelled. Cancelling the response for the
  // re-fetch above reports WebKitErrorFrameLoadInterruptedByPolicyChange (102), whose
  // constant is deprecated.
  const bool cancelled = error.code == NSURLErrorCancelled ||
                         ([error.domain isEqualToString:@"WebKitErrorDomain"] && error.code == 102);
  if (!cancelled)
    CLog::Log(LOGERROR, "HbbTV: navigation failed: {}", error.localizedDescription.UTF8String);
}

- (void)webView:(WKWebView*)webView didCommitNavigation:(WKNavigation*)navigation
{
  // A new document starts with the broadcast in full screen until it asks otherwise.
  CPVRGUIHbbTVBridge::OnNewDocument();
}

- (void)webView:(WKWebView*)webView didFinishNavigation:(WKNavigation*)navigation
{
  CLog::Log(LOGINFO, "HbbTV: loaded '{}'", webView.URL.absoluteString.UTF8String);
}

#pragma mark - WKScriptMessageHandler

- (void)userContentController:(WKUserContentController*)userContentController
      didReceiveScriptMessage:(WKScriptMessage*)message
{
  NSDictionary* body = message.body;
  CPVRGUIHbbTVBridge::LogConsole([body[@"text"] description].UTF8String,
                                 [body[@"error"] boolValue]);
}

@end

namespace
{
KodiHbbTVView* g_view = nil;
WKWebView* g_warmupView = nil; // keeps the web content process alive between applications
CPVRGUIHbbTVKeyRouter g_keyRouter;
NSEvent* g_pendingKey = nil; // key down kept until its release or until the key counts as held
bool g_replaying = false;
NSTimeInterval g_forwardedKeyDown = 0; // timestamp of the last key down given to the view

XBMC_keysym KeysymFor(NSEvent* event)
{
  auto* winSystem = dynamic_cast<CWinSystemOSX*>(CServiceBroker::GetWinSystem());
  return winSystem ? winSystem->TranslateKeyEvent(event) : XBMC_keysym{};
}

void ForwardToApplication(NSEvent* event, CPVRGUIHbbTVKeyRouter::Target target)
{
  if (target == CPVRGUIHbbTVKeyRouter::Target::APPLICATION)
  {
    if (event.type == NSEventTypeKeyDown)
      g_forwardedKeyDown = event.timestamp;
    [g_view forwardKeyEvent:event];
  }
  else
  {
    const int virtualKey = CPVRGUIHbbTVKeyRouter::VirtualKeyFor(KeysymFor(event).sym);
    [g_view dispatchKey:virtualKey type:(event.type == NSEventTypeKeyDown ? @"keydown" : @"keyup")];
  }
}

void CancelPendingKey()
{
  g_pendingKey = nil;
  g_keyRouter.Reset();
}
} // unnamed namespace

void CHbbTVWebView::Prepare()
{
  dispatch_async(dispatch_get_main_queue(), ^{
    if (g_warmupView)
      return;
    CLog::Log(LOGDEBUG, "HbbTV: starting web content process");
    WKWebViewConfiguration* config = [WKWebViewConfiguration new];
    config.processPool = ProcessPool();
    g_warmupView = [[WKWebView alloc] initWithFrame:NSZeroRect configuration:config];
    [g_warmupView loadHTMLString:@"" baseURL:nil];
  });
}

void CHbbTVWebView::Launch(const std::string& url)
{
  // Deferred: creating the view takes a while and must not stall Kodi's main loop.
  NSString* nsUrl = @(url.c_str());
  dispatch_async(dispatch_get_main_queue(), ^{
    if (g_view)
      [g_view close];

    NSWindow* parent = NSApp.mainWindow ?: NSApp.keyWindow ?: NSApp.windows.firstObject;
    if (!parent)
    {
      CLog::Log(LOGERROR, "HbbTV: no application window");
      return;
    }
    g_view = [[KodiHbbTVView alloc] initWithParent:parent url:nsUrl];
  });
}

void CHbbTVWebView::Close()
{
  // Always deferred: the request may originate from inside a bridge callback of the view.
  dispatch_async(dispatch_get_main_queue(), ^{
    if (!g_view)
      return;
    CLog::Log(LOGINFO, "HbbTV: closing application");
    CancelPendingKey();
    [g_view close];
    g_view = nil;
    CPVRGUIHbbTVBridge::ResetBroadcastPresentation();
  });
}

bool CHbbTVWebView::IsActive()
{
  return g_view != nil;
}

bool CHbbTVWebView::GetContentSize(float& width, float& height)
{
  if (!g_view)
    return false;
  width = static_cast<float>(g_view.contentSize.width);
  height = static_cast<float>(g_view.contentSize.height);
  return width > 0 && height > 0;
}

bool CHbbTVWebView::HandleKeyEvent(NSEvent* event)
{
  if (!g_view || g_replaying)
    return false;

  const bool down = event.type == NSEventTypeKeyDown;

  // WebKit resends a key down the page did not handle through NSApp. The application asked
  // for the key, so it is consumed either way; seen again it would count as a held key.
  if (down && event.timestamp == g_forwardedKeyDown)
    return true;

  CPVRGUIHbbTVKeyRouter::Key key;
  key.code = event.keyCode;
  key.keysym = KeysymFor(event);
  key.combination =
      (event.modifierFlags & (NSEventModifierFlagControl | NSEventModifierFlagOption |
                              NSEventModifierFlagCommand | NSEventModifierFlagShift)) != 0;
  // Escape is the exit key, which ends the application like on a TV set.
  key.exit = event.keyCode == 53;

  const auto target = g_keyRouter.OnKey(key, down, down && event.isARepeat);
  if (target == CPVRGUIHbbTVKeyRouter::Target::NONE)
    return true;

  if (target == CPVRGUIHbbTVKeyRouter::Target::HOLD)
  {
    g_pendingKey = event;
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, KODI::KEYBOARD::KEY_HOLD_TRESHOLD * NSEC_PER_MSEC),
        dispatch_get_main_queue(), ^{
          if (g_pendingKey != event)
            return;
          g_pendingKey = nil;
          g_keyRouter.OnKeyHeld();
        });
    return true;
  }

  const bool application = target != CPVRGUIHbbTVKeyRouter::Target::KODI;

  // A key that was kept until its release goes first.
  if (!down && g_pendingKey && g_pendingKey.keyCode == event.keyCode)
  {
    NSEvent* pending = g_pendingKey;
    g_pendingKey = nil;
    g_replaying = true;
    if (application)
      ForwardToApplication(pending, target);
    else
      [g_view.parent sendEvent:pending];
    g_replaying = false;
  }

  if (application)
    ForwardToApplication(event, target);
  return application;
}

void CHbbTVWebView::LoadUrl(const std::string& url)
{
  RunOnMainThread(^{
    if (g_view)
      [g_view loadUrl:@(url.c_str())];
  });
}

void CHbbTVWebView::RunScript(const std::string& script)
{
  RunOnMainThread(^{
    if (g_view)
      [g_view runScript:@(script.c_str())];
  });
}

void CHbbTVWebView::SendKey(int virtualKey)
{
  RunOnMainThread(^{
    if (!g_view)
      return;
    [g_view dispatchKey:virtualKey type:@"keydown"];
    [g_view dispatchKey:virtualKey type:@"keyup"];
  });
}

void CHbbTVWebView::SetVisible(bool visible)
{
  RunOnMainThread(^{
    if (!g_view)
      return;
    g_view.webView.hidden = !visible;
    if (visible)
    {
      // Regain the key status given up while hidden, otherwise the page stays inactive and
      // WebKit no longer renders :focus.
      [g_view.window makeKeyWindow];
      [g_view.window makeFirstResponder:g_view.webView];
    }
    else
    {
      // Keys go to Kodi while the application is hidden.
      [g_view.parent makeKeyWindow];
    }
  });
}

void CHbbTVEngineOSX::Register()
{
  CServiceBroker::GetPVRManager().Get<PVR::CPVRHbbTV>().SetEngine(
      std::make_shared<CHbbTVEngineOSX>());
}
