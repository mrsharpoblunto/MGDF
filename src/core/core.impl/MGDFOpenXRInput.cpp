#include "StdAfx.h"

#include <algorithm>

#include "MGDFOpenXR.hpp"

namespace MGDF::core {
namespace {
struct ActionDefinition {
  const char *Name;
  XrActionType Type;
  UINT32 Button = 0;
};
const ActionDefinition Actions[] = {
    {"grip", XR_ACTION_TYPE_POSE_INPUT},
    {"aim", XR_ACTION_TYPE_POSE_INPUT},
    {"trigger", XR_ACTION_TYPE_FLOAT_INPUT},
    {"squeeze", XR_ACTION_TYPE_FLOAT_INPUT},
    {"stick", XR_ACTION_TYPE_VECTOR2F_INPUT},
    {"a", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_BUTTON_A},
    {"b", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_BUTTON_B},
    {"x", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_BUTTON_X},
    {"y", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_BUTTON_Y},
    {"stick_click", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_BUTTON_STICK},
    {"menu", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_BUTTON_MENU},
    {"a_touch", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_TOUCH_A},
    {"b_touch", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_TOUCH_B},
    {"x_touch", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_TOUCH_X},
    {"y_touch", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_TOUCH_Y},
    {"stick_touch", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_TOUCH_STICK},
    {"trigger_touch", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_TOUCH_TRIGGER},
    {"thumbrest_touch", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_TOUCH_THUMBREST},
    {"trackpad_touch", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_TOUCH_TRACKPAD},
    {"trackpad_click", XR_ACTION_TYPE_BOOLEAN_INPUT, MGDF_VR_BUTTON_TRACKPAD},
    {"haptic", XR_ACTION_TYPE_VIBRATION_OUTPUT},
    {"gaze", XR_ACTION_TYPE_POSE_INPUT}};
const char *Profiles[] = {"/interaction_profiles/oculus/touch_controller",
                          "/interaction_profiles/valve/index_controller",
                          "/interaction_profiles/htc/vive_controller",
                          "/interaction_profiles/microsoft/motion_controller",
                          "/interaction_profiles/khr/simple_controller"};
}  // namespace

MGDFVRPose ConvertOpenXRPose(const XrPosef &pose, XrSpaceLocationFlags flags) {
  MGDFVRPose result{};
  result.PositionValid = (flags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;
  result.OrientationValid =
      (flags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0;
  if (result.PositionValid) {
    result.Position[0] = pose.position.x;
    result.Position[1] = pose.position.y;
    result.Position[2] = -pose.position.z;
  }
  if (result.OrientationValid) {
    result.Orientation[0] = -pose.orientation.x;
    result.Orientation[1] = -pose.orientation.y;
    result.Orientation[2] = pose.orientation.z;
    result.Orientation[3] = pose.orientation.w;
  }
  return result;
}

void OpenXR::CreateActions() {
  _inputStatus = E_FAIL;
  const auto check = [this](XrResult result) {
    return FeatureCheck(result, "create controller actions", _inputErrorLogged);
  };
  if (!check(xrStringToPath(_instance, "/user/hand/left", &_hands[0])) ||
      !check(xrStringToPath(_instance, "/user/hand/right", &_hands[1])))
    return;
  XrActionSetCreateInfo set{XR_TYPE_ACTION_SET_CREATE_INFO};
  strcpy_s(set.actionSetName, "mgdf");
  strcpy_s(set.localizedActionSetName, "MGDF controllers");
  if (!check(xrCreateActionSet(_instance, &set, &_actionSet))) return;
  static_assert(_countof(Actions) == ActionCount);
  for (UINT32 i = 0; i < ActionCount; ++i) {
    if (i == GazeAction && !_gazeSupported) continue;
    XrActionCreateInfo info{XR_TYPE_ACTION_CREATE_INFO};
    strcpy_s(info.actionName, Actions[i].Name);
    strcpy_s(info.localizedActionName, Actions[i].Name);
    info.actionType = Actions[i].Type;
    if (i != GazeAction) {
      info.countSubactionPaths = 2;
      info.subactionPaths = _hands.data();
    }
    if (!check(xrCreateAction(_actionSet, &info, &_actions[i]))) return;
  }
  for (UINT32 profile = 0; profile < _profilePaths.size(); ++profile) {
    if (!check(xrStringToPath(_instance, Profiles[profile],
                              &_profilePaths[profile])))
      return;
    std::vector<XrActionSuggestedBinding> bindings;
    bool valid = true;
    for (UINT32 hand = 0; hand < 2; ++hand) {
      const std::string user = hand ? "/user/hand/right/" : "/user/hand/left/";
      const auto bind = [&](UINT32 action, const char *component) {
        XrPath path = XR_NULL_PATH;
        if (!check(
                xrStringToPath(_instance, (user + component).c_str(), &path)))
          valid = false;
        else
          bindings.push_back({_actions[action], path});
      };
      bind(GripAction, "input/grip/pose");
      bind(AimAction, "input/aim/pose");
      bind(HapticAction, "output/haptic");
      if (profile == 4) {
        bind(TriggerAction, "input/select/click");
        bind(MenuAction, "input/menu/click");
      } else {
        bind(TriggerAction, "input/trigger/value");
        bind(SqueezeAction,
             profile < 2 ? "input/squeeze/value" : "input/squeeze/click");
        bind(StickAction, profile == 2 ? "input/trackpad" : "input/thumbstick");
        if (profile != 2) bind(StickClickAction, "input/thumbstick/click");
        if (profile == 0) {
          bind(hand ? AAction : XAction,
               hand ? "input/a/click" : "input/x/click");
          bind(hand ? BAction : YAction,
               hand ? "input/b/click" : "input/y/click");
          bind(hand ? ATouchAction : XTouchAction,
               hand ? "input/a/touch" : "input/x/touch");
          bind(hand ? BTouchAction : YTouchAction,
               hand ? "input/b/touch" : "input/y/touch");
          bind(ThumbrestTouchAction, "input/thumbrest/touch");
          if (!hand) bind(MenuAction, "input/menu/click");
        } else if (profile == 1) {
          bind(AAction, "input/a/click");
          bind(BAction, "input/b/click");
          bind(ATouchAction, "input/a/touch");
          bind(BTouchAction, "input/b/touch");
        } else
          bind(MenuAction, "input/menu/click");
        if (profile < 2) {
          bind(StickTouchAction, "input/thumbstick/touch");
          bind(TriggerTouchAction, "input/trigger/touch");
        }
        if (profile > 0) {
          bind(TrackpadTouchAction, "input/trackpad/touch");
          if (profile != 1) bind(TrackpadClickAction, "input/trackpad/click");
        }
      }
    }
    if (!valid) return;
    const XrInteractionProfileSuggestedBinding info{
        XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING, nullptr,
        _profilePaths[profile], static_cast<UINT32>(bindings.size()),
        bindings.data()};
    const auto result = xrSuggestInteractionProfileBindings(_instance, &info);
    if (result != XR_ERROR_PATH_UNSUPPORTED && !check(result)) return;
  }
  if (_gazeSupported) {
    XrPath profile, path;
    if (!check(xrStringToPath(_instance,
                              "/interaction_profiles/ext/eye_gaze_interaction",
                              &profile)) ||
        !check(xrStringToPath(_instance, "/user/eyes_ext/input/gaze_ext/pose",
                              &path)))
      return;
    const XrActionSuggestedBinding binding{_actions[GazeAction], path};
    const XrInteractionProfileSuggestedBinding info{
        XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING, nullptr, profile, 1,
        &binding};
    if (!check(xrSuggestInteractionProfileBindings(_instance, &info))) return;
  }
  const XrSessionActionSetsAttachInfo attach{
      XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO, nullptr, 1, &_actionSet};
  if (!check(xrAttachSessionActionSets(_session, &attach))) return;
  for (UINT32 hand = 0; hand < 2; ++hand) {
    for (UINT32 pose = 0; pose < 2; ++pose) {
      const XrActionSpaceCreateInfo info{XR_TYPE_ACTION_SPACE_CREATE_INFO,
                                         nullptr,
                                         _actions[pose],
                                         _hands[hand],
                                         {{0, 0, 0, 1}, {0, 0, 0}}};
      if (!check(xrCreateActionSpace(_session, &info,
                                     &_controllerSpaces[hand][pose])))
        return;
    }
  }
  if (_gazeSupported) {
    const XrActionSpaceCreateInfo info{XR_TYPE_ACTION_SPACE_CREATE_INFO,
                                       nullptr,
                                       _actions[GazeAction],
                                       XR_NULL_PATH,
                                       {{0, 0, 0, 1}, {0, 0, 0}}};
    if (!check(xrCreateActionSpace(_session, &info, &_gazeSpace))) return;
  }
  _inputStatus = S_OK;
}

void OpenXR::DestroyActions() {
  for (auto &hand : _controllerSpaces)
    for (auto &space : hand) {
      if (space) xrDestroySpace(space);
      space = XR_NULL_HANDLE;
    }
  if (_gazeSpace) xrDestroySpace(_gazeSpace);
  _gazeSpace = XR_NULL_HANDLE;
  if (_actionSet) xrDestroyActionSet(_actionSet);
  _actionSet = XR_NULL_HANDLE;
  _actions.fill(XR_NULL_HANDLE);
  _inputStatus = E_ILLEGAL_METHOD_CALL;
  _inputErrorLogged = _hapticErrorLogged = false;
  _inputFrame = {};
  _input->Publish(_inputFrame);
}

void OpenXR::UpdateInput() {
  _inputFrame = {};
  _inputFrame.ControllersStatus = _inputStatus;
  _inputFrame.GazeStatus = _gazeSupported ? _inputStatus : E_NOTIMPL;
  const auto publishEmpty = [&] { _input->Publish(_inputFrame); };
  if (FAILED(_inputStatus)) {
    publishEmpty();
    return;
  }
  _inputFrame.ControllersStatus = S_FALSE;
  _inputFrame.GazeStatus = _gazeSupported ? S_FALSE : E_NOTIMPL;
  if (!Focused()) {
    publishEmpty();
    return;
  }
  const auto check = [this](XrResult result) {
    return FeatureCheck(result, "read controller input", _inputErrorLogged);
  };
  const XrActiveActionSet active{_actionSet, XR_NULL_PATH};
  const XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO, nullptr, 1, &active};
  const auto result = xrSyncActions(_session, &sync);
  if (result == XR_SESSION_NOT_FOCUSED) {
    publishEmpty();
    return;
  }
  if (!check(result)) {
    _inputFrame.ControllersStatus = E_FAIL;
    if (_gazeSupported) _inputFrame.GazeStatus = E_FAIL;
    publishEmpty();
    return;
  }
  bool valid = true;
  const auto pose = [&](UINT32 action, XrPath hand, XrSpace space,
                        MGDFVRPose &output) {
    const XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO, nullptr,
                                   _actions[action], hand};
    XrActionStatePose state{XR_TYPE_ACTION_STATE_POSE};
    if (!check(xrGetActionStatePose(_session, &get, &state))) {
      valid = false;
      return false;
    }
    if (!state.isActive) return false;
    XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
    if (!check(xrLocateSpace(space, _space, _frame.predictedDisplayTime,
                             &location))) {
      valid = false;
      return false;
    }
    output = ConvertOpenXRPose(location.pose, location.locationFlags);
    return true;
  };
  for (UINT32 hand = 0; hand < 2; ++hand) {
    auto &controller = _inputFrame.Controllers[hand];
    bool connected = pose(GripAction, _hands[hand], _controllerSpaces[hand][0],
                          controller.Grip);
    connected |= pose(AimAction, _hands[hand], _controllerSpaces[hand][1],
                      controller.Aim);
    const auto scalar = [&](UINT32 action) {
      const XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO, nullptr,
                                     _actions[action], _hands[hand]};
      XrActionStateFloat state{XR_TYPE_ACTION_STATE_FLOAT};
      if (!check(xrGetActionStateFloat(_session, &get, &state))) valid = false;
      connected |= state.isActive != XR_FALSE;
      return state.isActive ? std::clamp(state.currentState, 0.0f, 1.0f) : 0.0f;
    };
    controller.Trigger = scalar(TriggerAction);
    controller.Squeeze = scalar(SqueezeAction);
    const XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO, nullptr,
                                   _actions[StickAction], _hands[hand]};
    XrActionStateVector2f stick{XR_TYPE_ACTION_STATE_VECTOR2F};
    if (!check(xrGetActionStateVector2f(_session, &get, &stick))) valid = false;
    if (stick.isActive) {
      connected = true;
      controller.StickX = std::clamp(stick.currentState.x, -1.0f, 1.0f);
      controller.StickY = std::clamp(stick.currentState.y, -1.0f, 1.0f);
    }
    for (UINT32 i = 5; i < HapticAction; ++i) {
      const XrActionStateGetInfo info{XR_TYPE_ACTION_STATE_GET_INFO, nullptr,
                                      _actions[i], _hands[hand]};
      XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
      if (!check(xrGetActionStateBoolean(_session, &info, &state)))
        valid = false;
      connected |= state.isActive != XR_FALSE;
      if (state.isActive && state.currentState)
        controller.Buttons |= Actions[i].Button;
    }
    if (connected) {
      controller.Connected = TRUE;
      XrInteractionProfileState profile{XR_TYPE_INTERACTION_PROFILE_STATE};
      if (!check(
              xrGetCurrentInteractionProfile(_session, _hands[hand], &profile)))
        valid = false;
      controller.Profile = MGDF_VR_PROFILE_OTHER;
      for (UINT32 i = 0; i < _profilePaths.size(); ++i)
        if (profile.interactionProfile == _profilePaths[i])
          controller.Profile = static_cast<MGDFVRControllerProfile>(i + 1);
    } else
      controller = {};
  }
  if (_gazeSupported) {
    const bool gazeActive =
        pose(GazeAction, XR_NULL_PATH, _gazeSpace, _inputFrame.Gaze);
    if (gazeActive && _inputFrame.Gaze.PositionValid &&
        _inputFrame.Gaze.OrientationValid)
      _inputFrame.GazeStatus = S_OK;
    else
      _inputFrame.Gaze = {};
  }
  if (!valid) {
    _inputFrame = {};
    _inputFrame.ControllersStatus = E_FAIL;
    _inputFrame.GazeStatus = _gazeSupported ? E_FAIL : E_NOTIMPL;
  } else
    _inputFrame.ControllersStatus = S_OK;
  _input->Publish(_inputFrame);
  const auto haptics = _input->TakeHaptics();
  for (UINT32 hand = 0; hand < 2; ++hand) {
    if (!haptics[hand] || !_inputFrame.Controllers[hand].Connected) continue;
    const auto &haptic = *haptics[hand];
    const XrHapticActionInfo info{XR_TYPE_HAPTIC_ACTION_INFO, nullptr,
                                  _actions[HapticAction], _hands[hand]};
    const XrHapticVibration vibration{
        XR_TYPE_HAPTIC_VIBRATION, nullptr,
        static_cast<XrDuration>(static_cast<double>(haptic.Seconds) *
                                1000000000.0),
        XR_FREQUENCY_UNSPECIFIED, haptic.Amplitude};
    FeatureCheck(
        haptic.Stop
            ? xrStopHapticFeedback(_session, &info)
            : xrApplyHapticFeedback(
                  _session, &info,
                  reinterpret_cast<const XrHapticBaseHeader *>(&vibration)),
        "controller haptics", _hapticErrorLogged);
  }
}

HRESULT OpenXR::GetControllerPose(MGDFVRHand hand, MGDFVRControllerPose kind,
                                  MGDFVRPose *pose) const {
  if (!pose) return E_INVALIDARG;
  *pose = {};
  if ((hand != MGDF_VR_HAND_LEFT && hand != MGDF_VR_HAND_RIGHT) ||
      (kind != MGDF_VR_CONTROLLER_GRIP && kind != MGDF_VR_CONTROLLER_AIM))
    return E_INVALIDARG;
  const auto status = SessionStatus();
  if (FAILED(status)) return status;
  if (!_begun || !_viewCount) return E_ILLEGAL_METHOD_CALL;
  if (FAILED(_inputFrame.ControllersStatus))
    return _inputFrame.ControllersStatus;
  const auto &controller = _inputFrame.Controllers[hand];
  if (!controller.Connected) return S_FALSE;
  *pose = kind == MGDF_VR_CONTROLLER_GRIP ? controller.Grip : controller.Aim;
  return pose->PositionValid || pose->OrientationValid ? S_OK : S_FALSE;
}

HRESULT OpenXR::GetGaze(MGDFVRPose *pose) const {
  if (!pose) return E_INVALIDARG;
  *pose = {};
  const auto status = SessionStatus();
  if (FAILED(status)) return status;
  if (!_begun || !_viewCount) return E_ILLEGAL_METHOD_CALL;
  if (_inputFrame.GazeStatus == S_OK) *pose = _inputFrame.Gaze;
  return _inputFrame.GazeStatus;
}

}  // namespace MGDF::core
