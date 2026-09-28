#pragma once

#include "config.h"
#include <optional>
#include <string_view>

namespace NitLink {

struct SettingsMessage {
    std::wstring action;
    std::wstring text;
    double number = 0;
    CaptureFormatOverride format;
};

// Only complete, bounded messages matching an action's schema cross the bridge.
std::optional<SettingsMessage> ParseSettingsMessage(std::wstring_view json);

} // namespace NitLink
