# Test runtime

Set `XR_RUNTIME_JSON` to the built `openxr.runtime.json` to run a game with
this runtime. It uses the default D3D12 adapter. Tests can select another
adapter with `Configure`.

These environment variables apply only to this test runtime. `Configure`
overrides them for core tests.

| Variable | Value |
| --- | --- |
| `MGDF_TEST_XR_TYPELESS_COLOR` | `1` creates typeless storage for sRGB color images. |
| `MGDF_TEST_XR_TYPELESS_DEPTH` | `1` creates typeless storage for depth images. |
| `MGDF_TEST_XR_SIMULTANEOUS_ACCESS` | `1` permits simultaneous access to color images. Depth images cannot use this D3D12 flag. |
| `MGDF_TEST_XR_EYE_WIDTH` | Recommended eye width in pixels. Default: 48 left, 64 right. |
| `MGDF_TEST_XR_EYE_HEIGHT` | Recommended eye height in pixels. Default: 48. |
| `MGDF_TEST_XR_READY_DELAY_MS` | Delay from session creation to READY, in milliseconds. Default: 0. |
| `MGDF_TEST_XR_REPORT` | File path for JSON frame, projection, image, and error counts. |

The report updates after the first frame, every 60 frames, and at session
destruction. A projection count greater than zero confirms eye submission.
