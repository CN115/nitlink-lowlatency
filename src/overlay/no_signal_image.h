#pragma once

#include <wincodec.h>
#include <cstdint>
#include <string>
#include <vector>

namespace NitLink {
// Encoded data stays in a read-only stream; only scaled pixels are allocated.
constexpr uint32_t kMaxImageFileBytes = 512 * 1024 * 1024;
enum class ImageLoadError { None, InvalidPath, NetworkLocation, FileUnavailable,
                            TooLarge, UnsupportedFormat, DecodeFailed, OutOfMemory };
const wchar_t* ImageLoadErrorKey(ImageLoadError error);
ImageLoadError NoSignalImageDrivePolicy(UINT drive);
struct ImageLoadDiagnostic {
    const wchar_t* operation = nullptr;
    HRESULT result = S_OK;
    UINT driveType = UINT_MAX;
};
constexpr uint32_t kMaxImagePixelBytes = 128 * 1024 * 1024;
struct DecodedImage {
    uint32_t width = 0, height = 0, stride = 0;
    std::vector<uint8_t> pixels;
};
bool DecodeNoSignalImage(IWICImagingFactory* factory, const std::wstring& path,
                         DecodedImage& output, ImageLoadError* error = nullptr,
                         ImageLoadDiagnostic* diagnostic = nullptr);
} // namespace NitLink
