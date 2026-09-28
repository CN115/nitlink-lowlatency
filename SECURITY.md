# Security

NitLink runs as a normal user process. It reaches capture hardware through Media Foundation and the vendor control paths documented in this repository, routes audio through WASAPI, and embeds a WebView2 panel that loads only the bundled settings page. The settings page is served from a fixed list of bundled assets, and its content policy blocks network connections. Approved links open in the user's browser only after a user-initiated action. Optional Discord Rich Presence uses local named pipes; the WebView2 runtime and Windows components have their own update and service behavior.

## Trust boundaries

Settings messages are checked against an exact document origin, a bounded schema, and a dispatch budget. Navigation, downloads, browser permissions, host objects, and unlisted resources are blocked. The same restrictions apply to elevated launches. The settings host refuses runtimes that lack required security interfaces and reports initialization failures visibly without hiding the capture picture.

Configuration, capture-frame metadata, audio formats, custom images, and IPC frames are validated before allocation or copying. Custom backgrounds accept local PNG, JPEG, and BMP files through explicit built-in decoders. Encoded files are streamed with a 512 MiB cap; large images are scaled before allocation to keep the application pixel buffer within 128 MiB. Source dimensions remain limited to 16384 per side. Network paths stay blocked and failures explain the reason. Frame storage is limited to 256 MiB per slot. Invalid inputs fail without being treated as another pixel or audio format.

These checks do not isolate the application from compromised drivers, Windows codecs, the WebView2 runtime, or someone who can replace the executable and its bundled assets. Keep those dependencies updated and run NitLink without administrator privileges. Hardware compatibility and device-driver behavior require testing on the target system.

Configuration saves normally replace a fully written sibling file without a synchronous disk flush. Sharing-related rename failures receive brief retries, then a guarded in-place fallback for ordinary single-link files with a recovery copy. Files that fail to load are preserved and saving is disabled for that session.

Release packaging checks the permitted file list and Microsoft signatures on any bundled Visual C++ runtime DLLs. The generated SHA256SUMS.txt records file integrity; a manifest shipped with the package is not publisher authentication. Application signing remains a separate release step.

## Reporting a vulnerability

Report privately through the **Report a vulnerability** button on this repository's Security tab rather than in a public issue. Include the NitLink version, the capture card, and steps to reproduce.

Reports get a reply on the advisory thread. A fix ships in the next release, with credit to the reporter unless they prefer otherwise.

## Supported versions

Only the latest release receives fixes.
