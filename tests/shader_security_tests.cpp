#include "../src/upscale/nis_upscaler.cpp"
#include <iostream>

int main() {
    NitLink::NisIncludeHandler include;
    LPCVOID data = nullptr; UINT bytes = 0;
    for (const char* name : {"../NIS_Scaler.h", "C:\\NIS_Scaler.h", "other.h", "NIS_Scaler.h/../x"})
        if (SUCCEEDED(include.Open(D3D_INCLUDE_LOCAL, name, nullptr, &data, &bytes))) return 1;
    if (FAILED(include.Open(D3D_INCLUDE_LOCAL, "NIS_Scaler.h", nullptr, &data, &bytes)) || !data || !bytes) return 1;
    include.Close(data);
    // Shader compilation is CPU-only. No D3D device or GPU is created.
    const D3D_SHADER_MACRO defines[] = {
        {"NIS_HLSL", "1"}, {"NIS_SCALER", "1"}, {"NIS_HDR_MODE", "0"}, {"NIS_USE_HALF_PRECISION", "1"},
        {"NIS_BLOCK_WIDTH", "32"}, {"NIS_BLOCK_HEIGHT", "24"}, {"NIS_THREAD_GROUP_SIZE", "128"}, {nullptr, nullptr}};
    Microsoft::WRL::ComPtr<ID3DBlob> shader, errors;
    const HRESULT hr = D3DCompile(NitLink::kNisMainHlsl, strlen(NitLink::kNisMainHlsl), "NIS_Main.hlsl",
        defines, &include, "main", "cs_5_0", 0, 0, &shader, &errors);
    if (FAILED(hr)) {
        if (errors) std::cerr << static_cast<const char*>(errors->GetBufferPointer());
        return 1;
    }
    std::cout << "Packaged shader include policy and CPU compilation passed\n";
    return 0;
}
