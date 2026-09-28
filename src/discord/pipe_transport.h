#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <array>

namespace NitLink {
// A local pipe server may identify the client, but cannot impersonate it.
HANDLE OpenRpcPipe(const wchar_t* name);
bool WriteRpcFrame(HANDLE pipe, HANDLE stop, uint32_t opcode, const std::string& payload,
                   DWORD timeoutMs = 1500);
bool ReadRpcFrame(HANDLE pipe, HANDLE stop, uint32_t& opcode, std::string& payload,
                  DWORD timeoutMs = 1500);

enum class RpcReadResult { Pending, Frame, Failed };
// An incomplete reply stays bounded and resumes on the next worker pass.
class RpcFrameReader {
public:
    RpcReadResult Poll(HANDLE pipe, HANDLE stop, uint32_t& opcode, std::string& payload);
    void Reset();
private:
    std::array<uint32_t, 2> m_header{};
    DWORD m_headerBytes = 0;
    std::string m_body;
    DWORD m_bodyBytes = 0;
};
} // namespace NitLink
