#include "pipe_transport.h"
#include <algorithm>
#include <cstring>
#include <vector>

namespace NitLink {
namespace {
constexpr uint32_t kMaxRpcPayload = 64 * 1024;

bool Transfer(HANDLE pipe, HANDLE stop, bool write, void* data, DWORD length, ULONGLONG deadline) {
    if (pipe == INVALID_HANDLE_VALUE || !stop) return false;
    HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!event) return false;
    bool success = true;
    auto* cursor = static_cast<uint8_t*>(data);
    while (length) {
        const auto now = GetTickCount64();
        if (now >= deadline || WaitForSingleObject(stop, 0) != WAIT_TIMEOUT) { success = false; break; }
        OVERLAPPED operation{};
        operation.hEvent = event;
        if (!ResetEvent(event)) { success = false; break; }
        DWORD transferred = 0;
        const BOOL done = write ? WriteFile(pipe, cursor, length, &transferred, &operation) :
                                  ReadFile(pipe, cursor, length, &transferred, &operation);
        if (!done) {
            if (GetLastError() != ERROR_IO_PENDING) { success = false; break; }
            HANDLE waits[] = {stop, event};
            const DWORD remaining = static_cast<DWORD>(std::min<ULONGLONG>(deadline - now, MAXDWORD - 1));
            const DWORD result = WaitForMultipleObjects(2, waits, FALSE, remaining);
            if (result != WAIT_OBJECT_0 + 1) {
                // The OVERLAPPED and its buffer must stay alive until the OS
                // acknowledges cancellation, even during application shutdown.
                CancelIoEx(pipe, &operation);
                GetOverlappedResult(pipe, &operation, &transferred, TRUE);
                success = false;
                break;
            }
            if (!GetOverlappedResult(pipe, &operation, &transferred, FALSE)) { success = false; break; }
        }
        if (!transferred || transferred > length) { success = false; break; }
        cursor += transferred;
        length -= transferred;
    }
    CloseHandle(event);
    return success;
}
} // namespace

HANDLE OpenRpcPipe(const wchar_t* name) {
    return CreateFileW(name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
}

bool WriteRpcFrame(HANDLE pipe, HANDLE stop, uint32_t opcode, const std::string& payload, DWORD timeoutMs) {
    if (opcode > 4 || payload.size() > kMaxRpcPayload) return false;
    const uint32_t header[] = {opcode, static_cast<uint32_t>(payload.size())};
    std::vector<uint8_t> frame(sizeof(header) + payload.size());
    std::memcpy(frame.data(), header, sizeof(header));
    if (!payload.empty()) std::memcpy(frame.data() + sizeof(header), payload.data(), payload.size());
    return Transfer(pipe, stop, true, frame.data(), static_cast<DWORD>(frame.size()), GetTickCount64() + timeoutMs);
}

bool ReadRpcFrame(HANDLE pipe, HANDLE stop, uint32_t& opcode, std::string& payload, DWORD timeoutMs) {
    opcode = 0;
    payload.clear();
    const auto deadline = GetTickCount64() + timeoutMs;
    uint32_t header[2]{};
    if (!Transfer(pipe, stop, false, header, sizeof(header), deadline) ||
        header[0] < 1 || header[0] > 4 || header[1] > kMaxRpcPayload) return false;
    std::string body(header[1], '\0');
    if (!body.empty() && !Transfer(pipe, stop, false, body.data(), header[1], deadline)) return false;
    opcode = header[0];
    payload = std::move(body);
    return true;
}

void RpcFrameReader::Reset() {
    m_header = {};
    m_headerBytes = m_bodyBytes = 0;
    m_body.clear();
}

RpcReadResult RpcFrameReader::Poll(HANDLE pipe, HANDLE stop, uint32_t& opcode, std::string& payload) {
    const auto failed = [&] { Reset(); return RpcReadResult::Failed; };
    opcode = 0;
    payload.clear();
    if (pipe == INVALID_HANDLE_VALUE || !stop || WaitForSingleObject(stop, 0) != WAIT_TIMEOUT)
        return failed();
    DWORD available = 0;
    if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr))
        return failed();
    const auto deadline = GetTickCount64() + 1500;
    if (m_headerBytes < sizeof(m_header)) {
        const DWORD count = std::min<DWORD>(available, sizeof(m_header) - m_headerBytes);
        if (count && !Transfer(pipe, stop, false,
            reinterpret_cast<BYTE*>(m_header.data()) + m_headerBytes, count, deadline))
            return failed();
        m_headerBytes += count;
        available -= count;
        if (m_headerBytes < sizeof(m_header)) return RpcReadResult::Pending;
        if (m_header[0] < 1 || m_header[0] > 4 || m_header[1] > kMaxRpcPayload)
            return failed();
        m_body.resize(m_header[1]);
    }
    const DWORD count = std::min<DWORD>(available, m_header[1] - m_bodyBytes);
    if (count && !Transfer(pipe, stop, false, m_body.data() + m_bodyBytes, count, deadline))
        return failed();
    m_bodyBytes += count;
    if (m_bodyBytes < m_header[1]) return RpcReadResult::Pending;
    opcode = m_header[0];
    payload = std::move(m_body);
    Reset();
    return RpcReadResult::Frame;
}
} // namespace NitLink
