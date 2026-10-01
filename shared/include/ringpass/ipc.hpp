#pragma once

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <ringpass/protocol.hpp>

namespace ringpass {

class SharedMemory {
public:
    SharedMemory() = default;
    ~SharedMemory() { close(); }

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    bool open_or_create()
    {
        if (state_)
            return true;

        mapping_ = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            static_cast<DWORD>(sizeof(SharedState)),
            kSharedMemoryName);

        if (!mapping_)
            return false;

        const bool created = GetLastError() != ERROR_ALREADY_EXISTS;

        state_ = static_cast<SharedState*>(
            MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedState)));

        if (!state_)
        {
            CloseHandle(mapping_);
            mapping_ = nullptr;
            return false;
        }

        if (created)
        {
            std::memset(state_, 0, sizeof(SharedState));
            state_->magic = kSharedMagic;
            state_->protocolVersion = kProtocolVersion;
            state_->sadx.protocolVersion = kProtocolVersion;
            state_->er.protocolVersion = kProtocolVersion;
        }

        return compatible();
    }

    bool open_existing(DWORD access = FILE_MAP_ALL_ACCESS)
    {
        if (state_)
            return true;

        mapping_ = OpenFileMappingA(access, FALSE, kSharedMemoryName);
        if (!mapping_)
            return false;

        state_ = static_cast<SharedState*>(
            MapViewOfFile(mapping_, access, 0, 0, sizeof(SharedState)));

        if (!state_)
        {
            CloseHandle(mapping_);
            mapping_ = nullptr;
            return false;
        }

        return compatible();
    }

    void close()
    {
        if (state_)
        {
            UnmapViewOfFile(state_);
            state_ = nullptr;
        }

        if (mapping_)
        {
            CloseHandle(mapping_);
            mapping_ = nullptr;
        }
    }

    [[nodiscard]] bool compatible() const
    {
        return state_ &&
               state_->magic == kSharedMagic &&
               state_->protocolVersion == kProtocolVersion;
    }

    SharedState* get() { return state_; }
    const SharedState* get() const { return state_; }

private:
    HANDLE mapping_{};
    SharedState* state_{};
};

template <typename Channel>
inline void begin_write(Channel& channel)
{
    ++channel.sequence;
    MemoryBarrier();
}

template <typename Channel>
inline void end_write(Channel& channel)
{
    MemoryBarrier();
    ++channel.sequence;
}

template <typename Channel>
inline bool read_stable(const Channel& source, Channel& out)
{
    for (int attempt = 0; attempt < 16; ++attempt)
    {
        const std::uint32_t before = source.sequence;
        if (before & 1u)
            continue;

        MemoryBarrier();
        std::memcpy(&out, const_cast<const void*>(
            reinterpret_cast<const volatile void*>(&source)), sizeof(Channel));
        MemoryBarrier();

        const std::uint32_t after = source.sequence;
        if (before == after && !(after & 1u))
            return true;
    }

    return false;
}

} // namespace ringpass
