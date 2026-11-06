# Render host and VR

Opt in through the [VR render setting](render-settings.md). In each `RTDraw`,
call `IMGDFRenderHost::GetViewCount()`. A count of `2` means that both eyes need
rendering. A count of `0` means that only the window needs rendering. This can
occur during an active session when the runtime does not request an eye frame.
Always draw the window.

The runtime can pause and resume the headset session. While it is idle,
`GetVR()` stays `TRUE`, focus is `FALSE`, and there are no eye views. The eye
targets are kept, and resuming the session causes no back buffer change.

For each index from zero to `GetViewCount() - 1`, call `GetView(index, &view)`.
Index `0` is the left eye and index `1` is the right eye. The pose is at this
frame's predicted display time in LOCAL space, in metres. It uses +X right,
+Y up, and +Z forward. The orientation quaternion is x, y, z, w. The field of
view angles are in radians, with left and down negative. `Width`, `Height`,
and `Format` describe that eye's target. Write linear color to an sRGB format.

In `RTBackBufferChange`, call `GetView` for indices 0 and 1 to read the new eye
sizes and formats and make eye-sized resources. Zero dimensions mean there are
no eye targets. Pose and field of view are valid only during `RTDraw` when
`GetViewCount()` is `2`.

Call `GetVRFocused()` to check headset input focus. Pause headset input when
it returns `FALSE`; do not stop the frame loop for this reason.

Use `GetViewTarget(index, &target)` on the selected D3D11 or D3D12 render host.
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

`GetView` and `GetViewTarget` return `S_OK` on success. Invalid indices or null
outputs return `E_INVALIDARG`. No session, a wrong thread, or a target request
outside an eye frame returns `E_ILLEGAL_METHOD_CALL`. A runtime failure returns
`E_FAIL`. Outputs are zero or null on error. `GetView` can also report target
properties outside `RTDraw`; its pose and field of view are then zero. During `RTDraw`
with no eye frame, it returns `E_ILLEGAL_METHOD_CALL`.

## Hidden area masks

Call `GetViewMaskVersion(&version)` on the render thread. Reload both masks
when the version changes. It changes after a mask event or recreation of the
session or eye targets. `GetViewMask(index, &vertexCount, xy, &indexCount,
indices)` uses two calls: null buffers return counts; provide those capacities
and buffers on the second call. Each vertex has two floats, and each triangle
has three 32-bit indices. The mesh covers the hidden area. It is counter-clockwise
in coordinates with x right and y up. Vertices are tangents at z=+1. Map x with
`ndc.x = (2*x - (tanRight + tanLeft)) / (tanRight - tanLeft)` and map y with
up and down in the same way, using the tangents of the `MGDFView` FOV angles.

A missing mask extension returns `E_NOTIMPL`; an empty mask returns `S_FALSE`
and zero counts. Invalid indices, null count pointers or insufficient capacity
return `E_INVALIDARG`; insufficient capacity still returns the required counts.
No session or the wrong thread returns `E_ILLEGAL_METHOD_CALL`. Runtime failure
returns `E_FAIL`. The version getter returns `S_OK`, `E_NOTIMPL`,
`E_INVALIDARG` for null output, `E_ILLEGAL_METHOD_CALL`, or `E_FAIL`.

## Reprojection depth

`MGDFView::DepthFormat` describes the optional depth target. `UNKNOWN` means
there is no depth target. `GetViewDepthTarget` returns an owned reference during
`RTDraw`, with the eye's dimensions. Release references and descriptors in
`RTBeforeBackBufferChange`. For D3D12 the image starts and must finish in
`D3D12_RESOURCE_STATE_DEPTH_WRITE`; submit work on the host's direct queue.
For D3D11 use the immediate context. Use `DepthFormat` to create a depth view.

After writing depth, call `SetViewDepthRange(index, nearZ, farZ)` for both eyes
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
the user. Pose uses the same axes and quaternion order as `MGDFView`. The host
copies the description. Changes take effect on the next eye frame; null hides
it immediately. A size change invalidates the old panel images at the next
`RTDraw`, so release references and cached descriptors by this `RTAfterPresent`.
Release them also in `RTBeforeBackBufferChange`.

Call `GetVRPanelTarget` during `RTDraw`. It returns an owned reference to the
current image, in `MGDFView::Format`. Write premultiplied alpha; transparent
pixels must have zero color. D3D12 images start and must finish in
`D3D12_RESOURCE_STATE_RENDER_TARGET`; use the direct queue. D3D11 uses the
immediate context. The visible panel is submitted after the eye projection.

Panel calls return `S_OK`, `E_NOTIMPL` if the runtime cannot show a panel,
`E_INVALIDARG` for invalid inputs, `E_ILLEGAL_METHOD_CALL` without a session
or on the wrong thread, or `E_FAIL` on runtime failure. The target getter also
requires an eye frame. It returns `S_FALSE` and null while hidden or before
the first panel frame, and `E_FAIL` if deferred image creation failed.

## Controllers, haptics, and gaze

On the sim thread, use `IMGDFInputManager::GetVRController(hand, &controller)`.
It returns `TRUE` when that hand is connected and has input focus. The result
has grip and aim poses, separate position and orientation validity, trigger
and squeeze values in 0..1, stick values in -1..1, button and touch bits, and
the active profile. Vive uses its trackpad for stick values. Missing controls
are zero. The host supports Touch, Index, Vive, WMR, and simple controller
bindings. A runtime can remap these controls.

`VibrateVRController(hand, amplitude, seconds)` queues vibration, with amplitude
in 0..1 and duration in (0,60] seconds. `StopVRControllerVibration(hand)` queues
a stop. The last request for each hand before the next display frame wins.
`TRUE` means queued; a later runtime failure is logged. Both return `FALSE`
for invalid arguments, no session, no focus, or a disconnected controller.
Queued vibration is discarded when focus or connection is lost.

`IMGDFInputManager::GetVRGaze(&pose)` returns `TRUE` only for a current gaze
sample with valid position and orientation. All four input-manager calls are
safe with VR off. Failed reads return `FALSE` and zero their output. Gaze is
unavailable when the runtime or headset lacks eye tracking.

During `RTDraw`, use `GetVRControllerPose(hand, gripOrAim, &pose)` and
`IMGDFRenderHost::GetVRGaze(&pose)` for this frame's predicted display time.
All poses use LOCAL space, metres, +X right, +Y up, +Z forward, and quaternion
x, y, z, w. Aim and gaze point along the rotated +Z direction. The calls return
`S_OK`, `S_FALSE` for no tracked sample or focus, `E_INVALIDARG` for invalid
inputs, `E_ILLEGAL_METHOD_CALL` without a session, outside an eye frame or on
the wrong thread, or `E_FAIL` on runtime failure. Gaze returns `E_NOTIMPL`
without eye tracking. Outputs are zero except on `S_OK`; check each validity
flag on a controller pose.
