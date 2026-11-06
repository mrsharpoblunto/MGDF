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
then call `Upscale` for the same view. The two eyes have separate histories.
When VR is off, `View` must be `0`. Frame generation and low latency sleep are
disabled in VR; their saved preferences are kept.

The test module requests VR when it is available. Each eye shows a color from
its pose, and the window shows the left eye color. F9 toggles VR. Escape exits.
Select D3D12 or D3D11 with the module's `host.graphicsAPI` preference, and start
with `-vr` to test startup in a headset.

## OpenXR access

Set `MGDFGraphicsRequirements::OpenXRExtensionCount` and `OpenXRExtensions` to
request extra extensions. Keep the array and its null-terminated names valid
for the module lifetime. The host enables listed names that the runtime supports
and ignores the rest. Use `IsOpenXRExtensionEnabled(name)` to check what the
current instance enabled, including host requirements. Recheck after back buffer
changes. This supports input, visibility masks, eye gaze and other extensions
without separate MGDF wrappers.

Call `GetOpenXRState(&state)` on either render backend. Cast `Instance`,
`SystemId`, `Session` and `LocalSpace` to their OpenXR types. Cast
`GetInstanceProcAddr` to `PFN_xrGetInstanceProcAddr` and use it to obtain OpenXR
functions. It is the loader's function; the module needs OpenXR headers and does
not link the loader. All fields are zero without a session. During an OpenXR
`RTDraw`, `PredictedDisplayTime` is the current `XrTime`; it is zero when no
OpenXR frame has begun. `LocalSpace` is the LOCAL space used for the eye poses.
Raw OpenXR data uses right-handed coordinates with -Z forward. It is not
converted to the left-handed `MGDFView` convention.

Make all module OpenXR calls on the render thread. Borrowed handles are valid
until the next `RTBeforeBackBufferChange` or device reset. Reacquire them in
`RTBackBufferChange`. A new session has new handles: rebuild session resources
and attach action sets again. A resize can keep the session; do not attach
sets twice to that session. Destroy only objects that the module owns, while
their parent handles are valid. Parent destruction also invalidates child
handles. Release eye target references at the back buffer boundary as above.

The host owns instance and session lifetime, graphics binding, adapter choice,
frame timing and eye swapchains. Do not call `xrWaitFrame`, `xrBeginFrame`,
`xrEndFrame`, `xrBeginSession`, `xrEndSession`, `xrRequestExitSession`,
`xrPollEvent`, or `xrDestroy*` on host-owned objects. Do not acquire, wait for,
or release host swapchain images. The module can create and manage its own
actions, spaces and swapchains through the supplied OpenXR functions.

Drain `PollOpenXREvent(&event, sizeof(event))` until it returns `FALSE`, where
`event` is an `XrEventDataBuffer`. Events arrive in poll order, including those
that the host also handles. Drain in `RTBeforeBackBufferChange` too, before
releasing OpenXR resources. The queue holds 64 whole buffers. On overflow the
oldest event is dropped, with one log line per instance. Undrained events are
discarded when the instance is destroyed. Event next chains point into the
output buffer; process them before reusing that buffer. A null or undersized
buffer returns `FALSE` without removing an event.

During `RTDraw` with two views, call `AddOpenXRCompositionLayers(count, layers)`
with an array of `const void*` pointing to `XrCompositionLayerBaseHeader`
structures. Calls append layers after the host projection layer, in call and
array order. Use `SetOpenXRProjectionViewNext(index, next)` to set each eye's
projection-view next chain, such as `XrCompositionLayerDepthInfoKHR`. A later
call for the same eye replaces the pointer; null clears it. Enable the required
extensions and obey the runtime's layer limit, including the host layer.
Finish rendering and release module swapchain images before `RTDraw` returns.
Keep layer structures, next chains and their data valid until `RTAfterPresent`.
All pointers are cleared each frame. Discarded frames submit no layers.

Both submission calls return `S_OK` on success and `E_UNEXPECTED` outside an
eye frame. Invalid indices, a null layer entry, or a null array with a nonzero
count return `E_INVALIDARG`. The layer pointer array is copied; its structures
and next chains remain owned by the module.
