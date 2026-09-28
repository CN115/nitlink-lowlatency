#pragma once
#include <windows.h>
#include <wrl.h>
#include <WebView2.h>

namespace NitLink {
inline bool SettingsProcessNeedsRestart(COREWEBVIEW2_PROCESS_FAILED_KIND kind) {
    return kind == COREWEBVIEW2_PROCESS_FAILED_KIND_BROWSER_PROCESS_EXITED ||
           kind == COREWEBVIEW2_PROCESS_FAILED_KIND_RENDER_PROCESS_EXITED;
}
constexpr unsigned kSettingsRestartLimit = 2;
constexpr unsigned long long kSettingsReadyTimeoutMs = 60000;
} // namespace NitLink
