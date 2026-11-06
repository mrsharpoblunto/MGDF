# Render host and VR

Opt in through the [VR render setting](render-settings.md). On the render
thread, call `IMGDFRenderHost::GetVRFrame(&frame)`. During `RTDraw`, `S_OK`
means `frame.EyeCount` is 2 and both eyes need rendering. `S_FALSE` means a VR
session exists but no eye frame is available; draw only the window. Always
draw the window, including when VR is off.

`MGDFVRFrame::Eyes[0]` is the left eye and `Eyes[1]` is the right eye. Their
poses are at this frame's predicted display time in LOCAL space, in metres,
with +X right, +Y up and +Z forward. The quaternion order is x, y, z, w. FOV
angles are in radians; left and down are negative. Width, Height, Format and
DepthFormat describe each target. Write linear color to an sRGB target.

In `RTBackBufferChange`, `GetVRFrame` returns `S_FALSE` with the target sizes
and formats, so the module can size resources. Outside an eye frame, EyeCount
is zero and all poses and FOV angles are zero. Check the validity flags of
Grip, Aim and Gaze during eye frames. Index Grip and Aim with `MGDFVRHand`.

The runtime can pause and resume the session. While idle, `GetVR()` stays
`TRUE`, Focused is `FALSE`, and EyeCount is zero. Targets are kept, and resume
causes no back buffer change. Pause headset input when Focused is `FALSE`;
continue rendering frames requested by the host.

Use `GetVREyeTarget(index, &target)` on the selected D3D11 or D3D12 render host.
The call returns an owned COM reference. Release it when it is no longer
needed. The target is valid for rendering during this `RTDraw`. The same images
rotate until the next back buffer change, so descriptors can be cached by
resource. Release all eye references and cached descriptors in
`RTBeforeBackBufferChange`.

For D3D12, each eye starts in `D3D12_RESOURCE_STATE_RENDER_TARGET`. Leave it in
that state when its command lists finish. Submit the eye work on the host's
native direct queue before `RTDraw` returns. Join any eye work on other queues
to that queue. The host records no commands on the eye image. For D3D11, submit
eye work on the immediate context before `RTDraw` returns.

For DLSS, set `View` to the eye index in both `MGDFFrameConstants` and
`MGDFUpscaleDesc`. Call `SetFrameConstants` once for each view in each frame,
then call `Upscale` for the same view. Views `0` and `1` are valid with or without
VR, each with a separate history. Either view may be omitted from a frame;
set constants for each view used in that frame. This also supports stereo in
the window without a headset. Frame generation and low latency sleep are disabled
in VR; their saved preferences are kept.

The test module requests VR when it is available. Each eye shows a color from
its pose, and the window shows the left eye color. A green panel follows the
head. Press a controller trigger to feel a short vibration; the active profile
is logged once per hand. F9 toggles VR. Escape exits.
Select D3D12 or D3D11 with the module's `host.graphicsAPI` preference, and start
with `-vr` to test startup in a headset.

VR resources and features are owned by the host. Modules use MGDF types and
calls; they do not call OpenXR or manage its handles or events.

`GetVRFrame` returns `E_INVALIDARG` for a null output,
`E_ILLEGAL_METHOD_CALL` without a session or on the wrong thread, and `E_FAIL`
after a runtime failure. Output is zero on error. `GetVREyeTarget` returns
`S_OK`, or the same errors; an invalid index returns `E_INVALIDARG`, and a
request outside an eye frame returns `E_ILLEGAL_METHOD_CALL`. Error targets
are null.

`MGDFVRFrame::Features` is a bit set of `MGDF_VR_FEATURE_MASK`, `_DEPTH`,
`_PANEL` and `_GAZE` for supported session features. Individual feature calls
can still fail. Gaze is invalid without a current sample or eye tracking.

## Hidden area masks

Read `MGDFVRFrame::MaskVersion` on the render thread. Reload both masks
when it changes. It changes after a mask event or recreation of the
session or eye targets. `GetVREyeMask(index, &vertexCount, xy, &indexCount,
indices)` uses two calls: null buffers return counts; provide those capacities
and buffers on the second call. Each vertex has two floats, and each triangle
has three 32-bit indices. The mesh covers the hidden area. It is counter-clockwise
in coordinates with x right and y up. Vertices are tangents at z=+1. Map x with
`ndc.x = (2*x - (tanRight + tanLeft)) / (tanRight - tanLeft)` and map y with
up and down in the same way, using the tangents of the `MGDFVREye` FOV angles.

A missing mask extension returns `E_NOTIMPL`; an empty mask returns `S_FALSE`
and zero counts. Invalid indices, null count pointers or insufficient capacity
return `E_INVALIDARG`; insufficient capacity still returns the required counts.
No session or the wrong thread returns `E_ILLEGAL_METHOD_CALL`. Runtime failure
returns `E_FAIL`. MaskVersion is zero without mask support.

## Reprojection depth

`MGDFVREye::DepthFormat` describes the optional depth target. `UNKNOWN` means
there is no depth target. `GetVREyeDepthTarget` returns an owned reference during
`RTDraw`, with the eye's dimensions. Release references and descriptors in
`RTBeforeBackBufferChange`. For D3D12 the image starts and must finish in
`D3D12_RESOURCE_STATE_DEPTH_WRITE`; submit work on the host's direct queue.
For D3D11 use the immediate context. Use `DepthFormat` to create a depth view.

After writing depth, call `SetVREyeDepthRange(index, nearZ, farZ)` for both eyes
in that frame. No depth is submitted unless both calls succeed. The distances
are in metres at depth values 0 and 1. For reverse-Z pass the distant plane
as `nearZ` and the close plane as `farZ`. Either distance can be positive
infinity; both must be positive and different. The calls return `S_OK`,
`E_NOTIMPL` without depth support, `E_INVALIDARG` for invalid inputs,
`E_ILLEGAL_METHOD_CALL` without a session, outside an eye frame or on the wrong
thread, or `E_FAIL` after runtime or resource failure. Target outputs are null
on error.

## Headset panel

Set one `MGDFVRPanel` with `SetVRPanel(&panel)` on the render thread. Give its
pixel dimensions, pose, size in metres, and `LOCAL` or `VIEW` space. LOCAL
stays in the room; VIEW follows the head. A panel at positive z is in front of
the user. Pose uses the same axes and quaternion order as `MGDFVREye`. The host
copies the description. Changes take effect on the next eye frame; null hides
it immediately. A size change invalidates the old panel images at the next
`RTDraw`, so release references and cached descriptors by this `RTAfterPresent`.
Release them also in `RTBeforeBackBufferChange`.

Call `GetVRPanelTarget` during `RTDraw`. It returns an owned reference to the
current image, in `MGDFVREye::Format`. Write premultiplied alpha; transparent
pixels must have zero color. D3D12 images start and must finish in
`D3D12_RESOURCE_STATE_RENDER_TARGET`; use the direct queue. D3D11 uses the
immediate context. The visible panel is submitted after the eye projection.

Panel calls return `S_OK`, `E_NOTIMPL` if the runtime cannot show a panel,
`E_INVALIDARG` for invalid inputs, `E_ILLEGAL_METHOD_CALL` without a session
or on the wrong thread, or `E_FAIL` on runtime failure. The target getter also
requires an eye frame. It returns `S_FALSE` and null while hidden or before
the first panel frame, and `E_FAIL` if deferred image creation failed.

## Controllers, haptics, and gaze

On the sim thread, call `IMGDFInputManager::GetVRControllerCount()` and
`GetVRControllers(controllers)`. The count is always 2, in left/right hand
order, including with VR off. Each returned `IMGDFVRController` is an owned
COM reference; release it when done. Keep these objects across session changes.

`GetHand` identifies the controller. `IsConnected` is false without input
focus or connection. `GetProfile` reports Touch, Index, Vive, WMR, simple,
other, or none. `IsButtonDown` and `IsButtonUp` read pressed buttons;
`IsTouched` reads the separate `MGDFVRTouch` enum. `IsButtonPress` returns true
once after a button is pressed and released, like the gamepad call. A press
observed between sim updates is kept for the next update.

`GetTrigger` and `GetSqueeze` return 0..1. `GetThumbX` and `GetThumbY` return
-1..1, with positive X right and positive Y up. The Y convention differs from
`IMGDFGamepad`. Vive uses its trackpad. `GetGripPose` and `GetAimPose` return
the current sim sample's LOCAL poses. Missing controls are zero. Without
focus, connection or a session, buttons are up, analog values are zero,
profile is none and poses are invalid. All controller calls are safe with VR off.

`SetVibration(amplitude)` stores a level until changed. Values are clamped to
0..1; NaN means zero. Set zero to stop. Output stops when focus, connection or
the session is lost, and resumes at the stored level when they return. Set
zero while inactive to prevent vibration from resuming.

`IMGDFInputManager::GetVRGaze(&pose)` returns `TRUE` only for a valid gaze
sample in the current sim update. It is safe with VR off. Without eye tracking,
focus, a session or a current sample, it returns `FALSE` and zeroes the output.

During `RTDraw`, use `MGDFVRFrame::Grip`, `Aim` and `Gaze` for poses at this
frame's predicted display time. All poses use LOCAL space, metres, +X right,
+Y up, +Z forward, and quaternion x, y, z, w. Aim and gaze point along rotated
+Z. Untracked or unavailable samples are invalid; check each validity flag.
