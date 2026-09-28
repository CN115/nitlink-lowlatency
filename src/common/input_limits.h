#pragma once

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace NitLink {

// These ceilings bound memory use before device metadata reaches allocation or
// row-copy arithmetic. They include 8K BGRA and the D3D11 dimension limit.
constexpr uint32_t kMaxVideoDimension = 16384;
constexpr uint64_t kMaxFrameBytes = 256ull * 1024 * 1024;
enum class PixelLayout { Bgra, Nv12, P010 };

inline bool FrameLayout(uint32_t width, uint32_t height, PixelLayout format,
                        uint32_t& rowBytes, uint32_t& frameBytes) {
    rowBytes = frameBytes = 0;
    if (!width || !height || width > kMaxVideoDimension || height > kMaxVideoDimension)
        return false;
    if (format != PixelLayout::Bgra && ((width | height) & 1)) return false;
    const uint64_t pixels = uint64_t(width) * height;
    const uint64_t bytes = format == PixelLayout::Bgra ? pixels * 4 :
                           format == PixelLayout::P010 ? pixels * 3 : pixels * 3 / 2;
    if (bytes > kMaxFrameBytes) return false;
    rowBytes = width * (format == PixelLayout::Bgra ? 4 : format == PixelLayout::P010 ? 2 : 1);
    frameBytes = static_cast<uint32_t>(bytes);
    return true;
}

inline uint32_t FrameCapacity(uint32_t width, uint32_t height, uint32_t stride) {
    uint32_t row = 0, bytes = 0;
    if (!stride || !FrameLayout(width, height, PixelLayout::Bgra, row, bytes)) return 0;
    const uint64_t capacity = std::max(uint64_t(bytes), uint64_t(stride) * height);
    return capacity <= kMaxFrameBytes ? static_cast<uint32_t>(capacity) : 0;
}

inline float ParseAspectRatio(std::string_view text) {
    auto trim = [](std::string_view value) {
        const auto first = value.find_first_not_of(" \t\r");
        if (first == std::string_view::npos) return std::string_view{};
        return value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
    };
    text = trim(text.substr(0, text.find('#')));
    if (text == "stretch") return -1.0f;
    const auto colon = text.find(':');
    if (colon == std::string_view::npos) return 0.0f;
    auto part = [trim](std::string_view value, float& number) {
        value = trim(value);
        if (value.empty()) return false;
        const auto result = std::from_chars(value.data(), value.data() + value.size(), number);
        return result.ec == std::errc{} && result.ptr != value.data() &&
            std::isfinite(number) && number >= 0.5f && number <= 100.0f;
    };
    float width = 0, height = 0;
    return part(text.substr(0, colon), width) && part(text.substr(colon + 1), height)
        ? width / height : 0.0f;
}

} // namespace NitLink
