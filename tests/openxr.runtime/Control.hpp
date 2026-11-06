#pragma once

#include <openxr/openxr.h>
#include <windows.h>

struct FakeXRConfig {
  LUID Adapter{};
  bool NoHeadset = false;
  bool ShouldRender = true;
  bool LossPendingOnAcquire = false;
  bool TimeoutFirstWait = false;
};
struct FakeXRStats {
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
using FakeXRShouldRender = void (*)(bool);
using FakeXRGetStats = void (*)(FakeXRStats *);
using FakeXRInstanceLoss = void (*)();
