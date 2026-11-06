#pragma once

#include <openxr/openxr.h>
#include <windows.h>

struct FakeXRConfig {
  LUID Adapter{};
  bool NoHeadset = false;
  bool ShouldRender = true;
  bool LossPendingOnAcquire = false;
  bool TimeoutFirstWait = false;
  bool DepthExtension = true;
  bool MaskExtension = true;
  bool PanelSupport = true;
  bool FailDepth = false;
  UINT32 FailWidth = 0;
};
struct FakeXRStats {
  XrApplicationInfo ApplicationInfo{};
  UINT32 EnabledExtensions = 0;
  bool DepthEnabled = false;
  bool MaskEnabled = false;
  UINT32 MaskReads = 0;
  XrReferenceSpaceType PanelSpace = XR_REFERENCE_SPACE_TYPE_LOCAL;
  UINT32 LastLayerCount = 0;
  UINT32 ExtraLayerFrames = 0;
  UINT64 LastLayers[3]{};
  XrCompositionLayerQuad LastQuads[3]{};
  UINT64 LastViewNext[2]{};
  XrCompositionLayerDepthInfoKHR LastDepth[2]{};
  UINT32 InstancesCreated = 0;
  UINT32 LiveInstances = 0;
  UINT32 SessionsCreated = 0;
  UINT32 LiveSessions = 0;
  UINT32 LiveSwapchains = 0;
  UINT32 SwapchainsCreated = 0;
  UINT32 Frames = 0;
  UINT32 ProjectionFrames = 0;
  UINT32 EmptyFrames = 0;
  UINT32 SubmittedImages = 0;
  UINT32 ExitRequests = 0;
  UINT32 SessionsBegun = 0;
  UINT32 SessionsEnded = 0;
  UINT32 Errors = 0;
};
using FakeXRConfigure = void (*)(const FakeXRConfig *);
using FakeXRPushState = void (*)(XrSessionState);
using FakeXRChangeMask = void (*)(UINT32);
using FakeXRShouldRender = void (*)(bool);
using FakeXRGetStats = void (*)(FakeXRStats *);
using FakeXRInstanceLoss = void (*)();
