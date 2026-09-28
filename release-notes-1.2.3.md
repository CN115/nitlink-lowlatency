# NitLink 1.2.3

Security hardening for the settings panel, saved settings, custom No Signal images and Discord Rich Presence, with clearer messages when something goes wrong.

## Security

- The settings panel loads only the bundled page and approved local assets. Strict origin, message, resource and content-policy checks block unwanted navigation, downloads, permissions, host objects and network requests.
- Configuration values, capture frames, audio formats, image sizes and Discord pipe messages are checked before allocation or copying. Numeric configuration values still accept a leading number followed by text, as in 1.2.2.
- DLL and shader loading are restricted and executable protections are enabled. Release packaging checks its file list and the signatures of the bundled Microsoft runtime files, and includes a `SHA256SUMS.txt` checksum list.

## Changes

- **The settings panel restarts after a crash.** If the WebView2 browser process or the menu page crashes, or the panel fails to start, NitLink restarts it up to two times per launch. Capture keeps running, and a message explains any failure.
- **Warnings when settings cannot be loaded or saved.** For example, when NitLink runs from a folder it cannot write to (such as under Program Files) or `nitlink.json` is read-only. Each warning appears once as a short toast and stays available in the F1 banner. Warnings are not included in NitLink screenshots.
- **Unreadable settings files are protected.** If `nitlink.json` cannot be read or is larger than 1 MiB, NitLink starts with default settings, leaves the original file untouched and does not save changes until the file is corrected or renamed and NitLink is restarted. When NitLink finds content it cannot use, it keeps up to two backups, `nitlink.json.bak` and `nitlink.json.bak.1`, each at most 1 MiB.
- **Safer saving.** Settings are written to a temporary file and then swapped in. If another program holds `nitlink.json`, NitLink retries briefly and can write in place, keeping a temporary `nitlink.json.save-recovery` copy of the previous settings. If a copy is left behind, your settings still load normally and the warning shows the copy's full path; NitLink automatically tries to remove a copy identical to your settings. If both the write and the restore fail, saving stops for that session and the message names the recovery file.
- **Custom No Signal images have clearer limits and messages.** PNG, JPEG and BMP files on a local drive are supported, up to 512 MiB and 16384 pixels per side. Large images are scaled down to stay within memory limits. When an image cannot be loaded, the message says why, and NitLink tries again after settings or graphics changes.
- **Discord reconnects after interruptions.** Local pipe operations have bounded waits and message sizes. Reconnect delays back off to a maximum of 30 seconds and reset after a healthy connection.

## Fixed

- **No Signal image controls.** A new Remove action clears the saved image path and returns to the default screen without deleting the file. Image-only rows are hidden while the default screen is selected, and long file names are shortened in the narrow panel. [#21](https://github.com/nitlink-dev/nitlink/pull/21)
- **Capture choices follow Resolution.** Frame rate and format are disabled while Resolution is set to Auto. [#21](https://github.com/nitlink-dev/nitlink/pull/21)
- **HDR warning while the source is unavailable.** Devices that detect HDR automatically, such as the GC553Pro, no longer show the manual-HDR warning while the HDMI source is temporarily unavailable. [#21](https://github.com/nitlink-dev/nitlink/pull/21)
- **Toast text wraps only when needed.** Notifications measure and draw text with the same layout, so a message that fits no longer breaks onto a second line. [#21](https://github.com/nitlink-dev/nitlink/pull/21)
- **Settings panel polish.** The narrow panel no longer shows an unneeded scrollbar, and control panes no longer select text by accident; About text stays selectable. [#21](https://github.com/nitlink-dev/nitlink/pull/21)

## Changed from 1.2.2

- Custom No Signal images on network shares or mapped network drives are no longer supported. Copy the image to a local drive and choose it again.
- Only PNG, JPEG and BMP images are accepted. Other formats that 1.2.2 could open through Windows codecs, such as GIF, TIFF or WebP, now show a message and the standard No Signal screen.
- The settings panel needs a current Microsoft Edge WebView2 Runtime. Windows normally keeps it up to date. If WebView2 updates are blocked and the runtime is too old, F1 shows a message instead of the panel; capture and hotkeys keep working.

## Contributors

Thanks to [HolyBear (聖小熊)](https://github.com/HolyBearTW) (`@HolyBearTW`) for the No Signal image, capture selector, HDR warning, toast and settings panel fixes in [#21](https://github.com/nitlink-dev/nitlink/pull/21).

## Install

Download `NitLink-1.2.3-win64.zip`, extract it into a new folder you can write to (not under Program Files) and run `NitLink.exe`. Keep all bundled files together beside the executable. Copy your existing `nitlink.json` into the new folder to retain your settings. A custom No Signal image must stay on a local drive at its saved path.

Requirements remain Windows 10 (1809+) or Windows 11, a DirectX 11 capable GPU, an up-to-date Microsoft Edge WebView2 Runtime and the Microsoft Visual C++ runtime (matching runtime DLLs are included). The binary is not code-signed.
