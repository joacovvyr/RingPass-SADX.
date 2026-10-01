#pragma once

#include <windows.h>

#include <cstdint>

#include <ringpass/protocol.hpp>

namespace ringpass {

enum class ChannelHealth {
    Offline,
    Stale,
    Incompatible,
    Live
};

inline ChannelHealth channel_health(
    std::uint32_t protocolVersion,
    std::uint64_t heartbeatMs)
{
    if (!heartbeatMs)
        return ChannelHealth::Offline;

    if (protocolVersion != kProtocolVersion)
        return ChannelHealth::Incompatible;

    const std::uint64_t now = GetTickCount64();

    if (now < heartbeatMs ||
        (now - heartbeatMs) > kHeartbeatTimeoutMs)
        return ChannelHealth::Stale;

    return ChannelHealth::Live;
}

inline const char* channel_health_name(ChannelHealth health)
{
    switch (health)
    {
    case ChannelHealth::Live: return "LIVE";
    case ChannelHealth::Stale: return "STALE";
    case ChannelHealth::Incompatible: return "INCOMPATIBLE";
    default: return "OFFLINE";
    }
}

} // namespace ringpass
