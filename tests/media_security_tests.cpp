// These tests exercise CPU-only helpers without opening an audio endpoint or
// capture device. Including the implementations keeps their internals local.
#include "../src/audio/audio_router.cpp"
#include "../src/capture/dshow_capture.cpp"
#include <iostream>
#include <stdexcept>

static void Check(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
class Sample final : public IMediaSample {
public:
    BYTE data[16]{};
    long length = 16;
    ULONG references = 1;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** value) override { if (value) *value = nullptr; return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references; }
    ULONG STDMETHODCALLTYPE Release() override { return --references; }
    HRESULT STDMETHODCALLTYPE GetPointer(BYTE** value) override { *value = data; return S_OK; }
    long STDMETHODCALLTYPE GetSize() override { return sizeof(data); }
    HRESULT STDMETHODCALLTYPE GetTime(REFERENCE_TIME*, REFERENCE_TIME*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetTime(REFERENCE_TIME*, REFERENCE_TIME*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE IsSyncPoint() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetSyncPoint(BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE IsPreroll() override { return S_FALSE; }
    HRESULT STDMETHODCALLTYPE SetPreroll(BOOL) override { return E_NOTIMPL; }
    long STDMETHODCALLTYPE GetActualDataLength() override { return length; }
    HRESULT STDMETHODCALLTYPE SetActualDataLength(long) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetMediaType(AM_MEDIA_TYPE**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetMediaType(AM_MEDIA_TYPE*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE IsDiscontinuity() override { return S_FALSE; }
    HRESULT STDMETHODCALLTYPE SetDiscontinuity(BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetMediaTime(LONGLONG*, LONGLONG*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetMediaTime(LONGLONG*, LONGLONG*) override { return E_NOTIMPL; }
};
namespace NitLink {
struct AudioRouterTestAccess {
    static void Run() {
        AudioRouter router;
        Check(!router.FifoReset(UINT32_MAX, UINT32_MAX), "FIFO allocation cap");
        Check(router.FifoReset(8, 48000), "valid FIFO");
        router.FifoPush(nullptr, 100000, true);
        Check(router.m_fifoBytes == router.m_fifo.size(), "oversized silent packet bounded");
        float samples[4] = {1, 1, 1, 1};
        Check(router.FifoPop(reinterpret_cast<BYTE*>(samples), 2) == 2 && samples[0] == 0 && samples[3] == 0, "silent FIFO produces zeroes");
        const auto size = router.m_fifoBytes;
        router.FifoPush(nullptr, 2, false);
        Check(size == router.m_fifoBytes, "null data not copied");
        router.SetVolume(0.5f); router.SetVolume(NAN);
        Check(router.m_volume == 0.5f, "NaN volume ignored");
    }
};
}

int main() {
    using namespace NitLink;
    try {
        WAVEFORMATEX wave{WAVE_FORMAT_IEEE_FLOAT, 2, 48000, 384000, 8, 32, 0};
        Check(ValidWaveFormat(&wave), "stereo float layout");
        float samples[4] = {1, -1, 0.5f, -0.5f};
        Check(ScaleInPlace(reinterpret_cast<BYTE*>(samples), 2, &wave, 0.5f) && samples[0] == 0.5f && samples[3] == -0.25f, "valid volume scaling");
        wave.nBlockAlign = 4;
        Check(!ValidWaveFormat(&wave) && !ScaleInPlace(reinterpret_cast<BYTE*>(samples), 2, &wave, 1), "short audio frame layout rejected");
        wave.nBlockAlign = 8; wave.nChannels = 65535;
        Check(!ValidWaveFormat(&wave), "channel cap");
        wave.nChannels = 2; wave.cbSize = 65535;
        Check(!ValidWaveFormat(&wave) && !CloneWaveFormat(&wave), "format extension cap");
        WAVEFORMATEXTENSIBLE ext{};
        ext.Format = {WAVE_FORMAT_EXTENSIBLE, 2, 48000, 288000, 6, 24, 22};
        ext.SubFormat = kSubtypePcm; ext.Samples.wValidBitsPerSample = 24;
        Check(ValidWaveFormat(&ext.Format), "24-bit extensible PCM supported");
        ext.Samples.wValidBitsPerSample = 32;
        Check(!ValidWaveFormat(&ext.Format), "invalid valid-bits field");
        AudioRouterTestAccess::Run();

        VIDEOINFOHEADER info{};
        info.bmiHeader.biWidth = 1920; info.bmiHeader.biHeight = -1080; info.AvgTimePerFrame = 166667;
        AM_MEDIA_TYPE mt{}; mt.majortype = MEDIATYPE_Video; mt.subtype = MFVideoFormat_NV12;
        mt.formattype = FORMAT_VideoInfo; mt.pbFormat = reinterpret_cast<BYTE*>(&info); mt.cbFormat = sizeof(info);
        Check(ValidNV12Type(&mt), "valid DirectShow NV12 metadata");
        info.bmiHeader.biHeight = LONG_MIN;
        Check(!ValidNV12Type(&mt), "LONG_MIN height rejected");
        info.bmiHeader.biHeight = 1080; info.bmiHeader.biWidth = 1921;
        Check(!ValidNV12Type(&mt), "odd planar dimensions rejected");
        info.bmiHeader.biWidth = 1920;
        AM_MEDIA_TYPE copy{};
        Check(SUCCEEDED(CopyMT(&copy, &mt)) && copy.pbFormat != mt.pbFormat && copy.cbFormat == sizeof(info), "media type deep copy");
        FreeMTContents(&copy);
        mt.cbFormat = 65537;
        Check(FAILED(CopyMT(&copy, &mt)) && !copy.pbFormat && !copy.pUnk, "oversized media format leaves empty destination");
        mt.cbFormat = sizeof(info); mt.pbFormat = nullptr;
        Check(FAILED(CopyMT(&copy, &mt)) && !ValidNV12Type(&mt), "missing media format bytes");
        Sample sample;
        mt.pbFormat = reinterpret_cast<BYTE*>(&info); mt.pUnk = &sample;
        Check(SUCCEEDED(CopyMT(&copy, &mt)) && sample.references == 2, "media type retains owned COM reference");
        FreeMTContents(&copy);
        Check(sample.references == 1, "media type releases exactly one reference");
        mt.cbFormat = 65537;
        Check(FAILED(CopyMT(&copy, &mt)) && sample.references == 1 && !copy.pUnk, "rejected copy does not acquire caller reference");
        DShowGraph graph;
        unsigned delivered = 0;
        graph.cb = [&](const uint8_t* data, uint32_t size, int64_t, int64_t, uint64_t) {
            Check(data == sample.data && size == sizeof(sample.data), "sample callback bounded to allocation");
            ++delivered;
        };
        sample.length = 17; graph.OnSample(&sample);
        sample.length = LONG_MIN; graph.OnSample(&sample);
        sample.length = LONG_MAX; graph.OnSample(&sample);
        Check(delivered == 0, "invalid sample lengths never reach callback");
        sample.length = 16; graph.OnSample(&sample);
        Check(delivered == 1, "normal sample after invalid samples");
        std::cout << "Audio and DirectShow metadata security tests passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
