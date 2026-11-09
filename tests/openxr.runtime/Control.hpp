#pragma once

#include <MGDF/MGDF.h>
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
  bool GazeExtension = false;
  bool EyeTracking = false;
  bool Api11 = false;
  bool Stage = false;
  bool LocalFloor = false;
  bool LocalFloorExtension = false;
  bool DeferStop = false;
  bool TypelessColor = false;
  bool TypelessDepth = false;
  bool SimultaneousAccess = false;
  UINT32 EyeWidth = 0;
  UINT32 EyeHeight = 0;
  UINT32 ReadyDelayMs = 0;
  bool DeferReady = false;
};
struct FakeXRInput {
  bool Connected[2]{true, true};
  FLOAT Trigger[2]{0.75f, 0.25f};
  FLOAT Squeeze[2]{0.4f, 0.6f};
  XrVector2f Stick[2]{{0.2f, -0.3f}, {-0.4f, 0.5f}};
  UINT32 Buttons[2]{MGDF_VR_BUTTON_X, MGDF_VR_BUTTON_A | MGDF_VR_BUTTON_STICK};
  UINT32 Touches[2]{MGDF_VR_TOUCH_X | MGDF_VR_TOUCH_TRIGGER, MGDF_VR_TOUCH_A};
  XrPosef Grip[2]{{{0, 0.6f, 0, 0.8f}, {-0.3f, 1.2f, -0.5f}},
                  {{0, -0.6f, 0, 0.8f}, {0.3f, 1.2f, -0.5f}}};
  XrPosef Aim[2]{{{0.6f, 0, 0, 0.8f}, {-0.3f, 1.3f, -0.6f}},
                 {{0.6f, 0, 0, 0.8f}, {0.3f, 1.3f, -0.6f}}};
  XrSpaceLocationFlags PoseFlags = XR_SPACE_LOCATION_POSITION_VALID_BIT |
                                   XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
  bool GazeActive = true;
  XrPosef Gaze{{0, 0.6f, 0, 0.8f}, {0.1f, 1.7f, -0.2f}};
  bool FailSync = false;
  bool FloorValid = true;
  float FloorHeight = -1.7f;
  float ChangedFloorHeight = -1.5f;
  XrTime FloorChangeTime = 33000000;
};
struct FakeXRStats {
  XrApplicationInfo ApplicationInfo{};
  UINT32 EnabledExtensions = 0;
  bool DepthEnabled = false;
  bool MaskEnabled = false;
  bool GazeEnabled = false;
  UINT32 ActionSetsCreated = 0;
  UINT32 ActionSetsAttached = 0;
  UINT32 SuggestedProfiles = 0;
  UINT32 ActionsSynced = 0;
  UINT32 HapticsApplied = 0;
  UINT32 HapticsStopped = 0;
  UINT32 LastHapticHand = 0;
  XrHapticVibration LastHaptic{};
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
using FakeXRChangeSpace = void (*)(XrReferenceSpaceType, XrTime);
using FakeXRChangeMask = void (*)(UINT32);
using FakeXRShouldRender = void (*)(bool);
using FakeXRGetStats = void (*)(FakeXRStats *);
using FakeXRInstanceLoss = void (*)();

using FakeXRSetInput = void (*)(const FakeXRInput *);

using FakeXRFailAcquire = void (*)();
