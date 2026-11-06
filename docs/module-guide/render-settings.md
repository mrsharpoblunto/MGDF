# Render settings and VR

Set `MGDFGraphicsRequirements::VR = TRUE` only if the module renders both eye
views. A zero-initialized requirements struct keeps VR unavailable. The window
must still be drawn in each frame.

Read `IMGDFRenderSettingsManager::GetVRSupport()` before you offer a VR option.
It returns `FALSE` if the module did not opt in or no active OpenXR runtime
supports its graphics API. A `TRUE` result does not mean a headset is connected.

To change VR, create an `IMGDFPendingRenderSettingsChange`, call `SetVR(TRUE)`
or `SetVR(FALSE)`, then release the change. `SetVR` returns `TRUE` when accepted,
or `FALSE` if VR is unavailable. The change uses `RTBeforeBackBufferChange` and
`RTBackBufferChange`. A change of graphics adapter can also cause a device
reset. Release device resources in `RTBeforeDeviceReset`, then call
`QueueDeviceReset` as usual.

The preference is `host.vr`, with a default value of `0`. Start `core.exe` with
`-vr` to request VR for that run without changing the saved value. A module
that did not opt in ignores both the preference and the flag.

`GetVR()` returns `TRUE` while VR mode has a headset session, including when the
runtime pauses it. During a pause, the window sets the frame rate and
`GetVRFrame` returns `S_FALSE` with `EyeCount` zero. When the runtime is ready again, eye rendering
resumes with the same targets and no back buffer change.

If the headset is unavailable, startup fails, or the runtime exits or loses the
session, VR mode ends and the game continues in the window. The saved preference
stays unchanged. A new `SetVR(TRUE)` request can retry after the headset becomes
available.

Frame generation is off in VR, and `GetMaxGeneratedFrames()` returns `0`.
Low latency sleep and frame markers have no effect in VR. The saved frame
generation and low latency settings apply again when VR ends. DLSS upscaling
can run separately for each eye.

See [the render host contract](render-host.md) for eye rendering.
