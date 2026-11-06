# Render host and VR

Opt in through the [VR render setting](render-settings.md). In each `RTDraw`,
call `IMGDFRenderHost::GetViewCount()`. A count of `2` means that both eyes need
rendering. A count of `0` means that only the window needs rendering. This can
occur during an active session when the runtime does not request an eye frame.
Always draw the window.

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
