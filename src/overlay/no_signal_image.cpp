#include "no_signal_image.h"
#include <wrl/client.h>
#include <shlwapi.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace NitLink {
using Microsoft::WRL::ComPtr;

const wchar_t* ImageLoadErrorKey(ImageLoadError error) {
    switch (error) {
    case ImageLoadError::InvalidPath: return L"toast.imageInvalidPath";
    case ImageLoadError::NetworkLocation: return L"toast.imageNetworkLocation";
    case ImageLoadError::FileUnavailable: return L"toast.imageFileUnavailable";
    case ImageLoadError::TooLarge: return L"toast.imageTooLarge";
    case ImageLoadError::UnsupportedFormat: return L"toast.imageUnsupportedFormat";
    case ImageLoadError::OutOfMemory: return L"toast.imageOutOfMemory";
    default: return L"toast.noSignalImageLoadFailed";
    }
}

ImageLoadError NoSignalImageDrivePolicy(UINT drive) {
    if (drive == DRIVE_REMOTE) return ImageLoadError::NetworkLocation;
    if (drive == DRIVE_FIXED || drive == DRIVE_REMOVABLE || drive == DRIVE_RAMDISK || drive == DRIVE_CDROM)
        return ImageLoadError::None;
    return ImageLoadError::FileUnavailable;
}

bool DecodeNoSignalImage(IWICImagingFactory* factory, const std::wstring& path,
                         DecodedImage& output, ImageLoadError* error, ImageLoadDiagnostic* diagnostic) {
    output = {};
    if (diagnostic) *diagnostic = {};
    const auto failed = [&](HRESULT hr, const wchar_t* operation) {
        if (SUCCEEDED(hr)) return false;
        if (diagnostic) *diagnostic = {operation, hr};
        return true;
    };
    if (error) *error = ImageLoadError::None;
    const auto fail = [&](ImageLoadError reason) {
        if (error) *error = reason;
        return false;
    };
    // Network and device locations remain outside the local-image contract.
    if (path.starts_with(L"\\\\") && !path.starts_with(L"\\\\?\\") && !path.starts_with(L"\\\\.\\"))
        return fail(ImageLoadError::NetworkLocation);
    if (!factory) return fail(ImageLoadError::DecodeFailed);
    if (path.size() < 4 || path.size() > 32767 || path[1] != L':' ||
        (path[2] != L'\\' && path[2] != L'/') ||
        !((path[0] >= L'A' && path[0] <= L'Z') || (path[0] >= L'a' && path[0] <= L'z')))
        return fail(ImageLoadError::InvalidPath);
    for (size_t i = 2; i < path.size(); ++i)
        if (path[i] < 0x20 || path[i] == L':' || path[i] == L'"') return fail(ImageLoadError::InvalidPath);
    const wchar_t root[] = {path[0], L':', L'\\', 0};
    const UINT drive = GetDriveTypeW(root);
    const auto driveError = NoSignalImageDrivePolicy(drive);
    if (driveError != ImageLoadError::None) {
        if (diagnostic) diagnostic->driveType = drive;
        return fail(driveError);
    }

    try {
        // Hold the same read-only stream throughout inspection and decoding.
        // Large BMP/PNG files need no full encoded copy in application memory.
        ComPtr<IStream> stream;
        if (failed(SHCreateStreamOnFileEx(path.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE,
                                         FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &stream), L"SHCreateStreamOnFileEx"))
            return fail(ImageLoadError::FileUnavailable);
        STATSTG stat{};
        if (failed(stream->Stat(&stat, STATFLAG_NONAME), L"IStream::Stat")) return fail(ImageLoadError::FileUnavailable);
        if (stat.cbSize.QuadPart > kMaxImageFileBytes) return fail(ImageLoadError::TooLarge);
        if (stat.cbSize.QuadPart < 8) return fail(ImageLoadError::DecodeFailed);
        uint8_t signature[8]{};
        ULONG read = 0;
        if (failed(stream->Read(signature, sizeof(signature), &read), L"IStream::Read") || read != sizeof(signature) ||
            failed(stream->Seek({}, STREAM_SEEK_SET, nullptr), L"IStream::Seek")) return fail(ImageLoadError::DecodeFailed);

        // Explicit built-in decoders prevent extensions or registered codecs
        // from selecting additional code to load into the viewer.
        CLSID decoderId;
        constexpr uint8_t png[] = {137, 80, 78, 71, 13, 10, 26, 10};
        if (std::memcmp(signature, png, sizeof(png)) == 0) decoderId = CLSID_WICPngDecoder;
        else if (signature[0] == 0xff && signature[1] == 0xd8 && signature[2] == 0xff) decoderId = CLSID_WICJpegDecoder;
        else if (signature[0] == 'B' && signature[1] == 'M') decoderId = CLSID_WICBmpDecoder;
        else return fail(ImageLoadError::UnsupportedFormat);

        ComPtr<IWICBitmapDecoder> decoder;
        ComPtr<IWICBitmapFrameDecode> frame;
        if (failed(CoCreateInstance(decoderId, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&decoder)), L"CoCreateInstance(decoder)") ||
            failed(decoder->Initialize(stream.Get(), WICDecodeMetadataCacheOnDemand), L"IWICBitmapDecoder::Initialize") ||
            failed(decoder->GetFrame(0, &frame), L"IWICBitmapDecoder::GetFrame")) return fail(ImageLoadError::DecodeFailed);
        DecodedImage image;
        if (failed(frame->GetSize(&image.width, &image.height), L"IWICBitmapFrameDecode::GetSize") || !image.width || !image.height)
            return fail(ImageLoadError::DecodeFailed);
        if (image.width > 16384 || image.height > 16384) return fail(ImageLoadError::TooLarge);

        ComPtr<IWICBitmapScaler> scaler;
        IWICBitmapSource* source = frame.Get();
        const uint64_t fullBytes = uint64_t(image.width) * image.height * 4;
        if (fullBytes > kMaxImagePixelBytes) {
            const double scale = std::sqrt(double(kMaxImagePixelBytes) / double(fullBytes));
            image.width = std::max(1u, static_cast<uint32_t>(std::floor(image.width * scale)));
            image.height = std::max(1u, static_cast<uint32_t>(std::floor(image.height * scale)));
            if (failed(factory->CreateBitmapScaler(&scaler), L"CreateBitmapScaler") ||
                failed(scaler->Initialize(frame.Get(), image.width, image.height, WICBitmapInterpolationModeFant), L"IWICBitmapScaler::Initialize"))
                return fail(ImageLoadError::DecodeFailed);
            source = scaler.Get();
        }
        image.stride = image.width * 4;
        const uint64_t bytes = uint64_t(image.stride) * image.height;
        if (bytes > kMaxImagePixelBytes) return fail(ImageLoadError::TooLarge);

        ComPtr<IWICFormatConverter> converter;
        if (failed(factory->CreateFormatConverter(&converter), L"CreateFormatConverter") ||
            failed(converter->Initialize(source, GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom), L"IWICFormatConverter::Initialize"))
            return fail(ImageLoadError::DecodeFailed);
        image.pixels.resize(static_cast<size_t>(bytes));
        if (failed(converter->CopyPixels(nullptr, image.stride, static_cast<UINT>(bytes), image.pixels.data()), L"IWICFormatConverter::CopyPixels"))
            return fail(ImageLoadError::DecodeFailed);
        output = std::move(image);
        return true;
    } catch (...) {
        if (diagnostic) *diagnostic = {L"image allocation", E_OUTOFMEMORY};
        return fail(ImageLoadError::OutOfMemory);
    }
}
} // namespace NitLink
