# NitLink 1.2.2

New logo and Windows app icon, blue accents in the existing free UI, and all public fixes since 1.2.1. The existing free UI is retained, with Color Range Expansion moved into Advanced when applicable.

## New

- **AVerMedia Live Gamer ULTRA S GC553Pro support.** Native P010 capture enables HDR10 through the card's Media Foundation modes. In Auto capture mode, NitLink now reads the HDMI source HDR state and selects P010 for confirmed HDR10/PQ or NV12 for SDR. Source transitions settle before formats change; failed probes retain the last valid state. Manual capture formats and the HDR output preference remain independent. Until source detection succeeds, the existing P010/HDR-to-SDR fallback remains available. See [#12](https://github.com/nitlink-dev/nitlink/pull/12) and [#20](https://github.com/nitlink-dev/nitlink/pull/20).
- **Traditional Chinese localization.** Settings, the HUD, shortcuts and capture notices support English and Traditional Chinese. The language choice is saved, System Default follows supported Windows language settings, and missing translations fall back to English. Both locale files are included in the package. [#11](https://github.com/nitlink-dev/nitlink/pull/11)
- **Custom No Signal images.** Choose a PNG, JPEG/JPG or BMP in F1 > Video > No Signal, with Contain, Cover or Stretch and optional dimming. The branded screen remains the default and the fallback when an image cannot be loaded. Images are cached and restored after graphics-resource recreation. [#19](https://github.com/nitlink-dev/nitlink/pull/19)
- **Prevent sleep while video is visible.** Enabled by default, including paused games. The request is released when minimized, hidden, showing No Signal, covered by the full settings panel or closed. Disable Prevent sleep in F1 to use normal Windows idle behavior. Power plans, manual sleep and secure screen savers are unchanged. [#13](https://github.com/nitlink-dev/nitlink/issues/13)

## Fixed

- **VSync is available again.** `Alt+V` and the F1 switch synchronize presentation on fixed-refresh displays. The choice defaults to off, saves immediately, survives restart and graphics-device recovery, and stays in sync between the menu and hotkey. It is separate from Low-Latency Mode and Present Pacing. VSync bypasses the tearing-mode frame cap and can add input delay; GPU driver overrides still apply.
- **HDR-to-SDR output preserves color more consistently.** P010 previews and SDR screenshots use the same luminance-based tone mapping and gamut compression instead of separate per-channel compression. HDR10 passthrough remains intact. [#9](https://github.com/nitlink-dev/nitlink/pull/9)
- **GC553Pro HDR output switches no longer reopen capture unnecessarily.** P010 capture can remain active when switching between HDR output and tone-mapped SDR output. Explicit format choices are honored, and accepted fallback state belongs only to the current device session. [#14](https://github.com/nitlink-dev/nitlink/pull/14)
- **Fractional frame rates are preserved.** Native rates such as 59.94 remain exact through format selection, saved preferences and driver readback. Older integer preferences resolve against the device's native modes, and frame-rate labels are easier to read. [#14](https://github.com/nitlink-dev/nitlink/pull/14)
- **Startup and capture recovery avoid stale pictures.** A waiting screen remains visible until a presentable capture frame arrives. GC553Pro placeholder handling preserves valid black frames, loading screens and moving content, with a limited startup waiting hint. [#16](https://github.com/nitlink-dev/nitlink/pull/16)
- **Source frame rate pacing stays responsive while placeholders are filtered.** Filtered samples wake the loop without exposing placeholder images as readable capture frames or changing signal detection. The retained-cadence fix from 1.2.1 remains included. [#19](https://github.com/nitlink-dev/nitlink/pull/19)
- **F1 responds faster during ordinary state updates.** Capture-device names are cached instead of repeatedly enumerated. Keyboard focus states and localized capture-mode notices are clearer. [#19](https://github.com/nitlink-dev/nitlink/pull/19)
- **The last normal window size is saved correctly on close.** Minimized, maximized, fullscreen and PiP dimensions no longer replace the normal window size.

## Changed

- Removed the Hardware Scaler row, which offered no alternative setting. Catmull-Rom scaling remains automatic.

- Color Range Expansion appears under F1 > Video > Advanced only when it can affect SDR RGB capture. Its saved preference is retained when the control is hidden.

- The Windows icon and in-app branding use the new NitLink logo. Blue accents replace the previous amber branding while keeping the free UI's existing arrangement and functionality.

## Known limitations

GC553Pro capture uses only native modes reported by the driver. Manual 1920x1080@60 P010 was validated on Windows 11, and the contributor supplied hardware validation for later GC553Pro fixes; broader automatic native-mode fallback selection has not been fully validated on that hardware. VSync can add input delay, and Windows HDR must be enabled on the display for HDR10 output.

## Contributors

Thanks to [HolyBear (聖小熊)](https://github.com/HolyBearTW) (`@HolyBearTW`) for HDR-to-SDR improvements, Traditional Chinese localization, GC553Pro support and hardware validation, fractional-rate and startup fixes, custom No Signal images, and settings responsiveness work in [#9](https://github.com/nitlink-dev/nitlink/pull/9), [#11](https://github.com/nitlink-dev/nitlink/pull/11), [#12](https://github.com/nitlink-dev/nitlink/pull/12), [#14](https://github.com/nitlink-dev/nitlink/pull/14), [#16](https://github.com/nitlink-dev/nitlink/pull/16), [#19](https://github.com/nitlink-dev/nitlink/pull/19) and [#20](https://github.com/nitlink-dev/nitlink/pull/20).

## Install

Download `NitLink-1.2.2-win64.zip`, extract it into a new folder and run `NitLink.exe`. Keep `nitlink-menu.html`, the `locales` folder and the other runtime files beside the executable. Copy your existing `nitlink.json` into the new folder to retain your settings. A custom No Signal image must remain accessible at its saved path.

Requirements remain Windows 10 (1809+) or Windows 11, a DirectX 11 capable GPU, Microsoft Edge WebView2 and the Microsoft Visual C++ runtime (matching runtime DLLs are included). The binary is not code-signed.
