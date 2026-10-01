#include <windows.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "SADXModLoader.h"
#include "target_proxy_adapter.hpp"

#include <ringpass/health.hpp>
#include <ringpass/ipc.hpp>
#include <ringpass/logger.hpp>

namespace {

ringpass::SharedMemory g_ipc;
std::uint64_t g_frame = 0;

std::unique_ptr<ringpass::Logger> g_logger;
ringpass::sadx::TargetProxyAdapter g_targetAdapter;
std::uint32_t g_lastPreparedTargetCount = 0xFFFFFFFFu;

ringpass::CharacterId ToRingPassCharacter(int sadxCharacter)
{
    switch (sadxCharacter)
    {
    case Characters_Sonic: return ringpass::CharacterId::Sonic;
    case Characters_Tails: return ringpass::CharacterId::Tails;
    case Characters_Knuckles: return ringpass::CharacterId::Knuckles;
    case Characters_Amy: return ringpass::CharacterId::Amy;
    case Characters_Gamma: return ringpass::CharacterId::Gamma;
    case Characters_Big: return ringpass::CharacterId::Big;
    default: return ringpass::CharacterId::Unknown;
    }
}

void Log(const std::string& line)
{
    if (g_logger)
        g_logger->write(line);
}

void PublishPlayerState()
{
    if (!g_ipc.open_or_create())
        return;

    auto& channel = g_ipc.get()->sadx;
    ringpass::begin_write(channel);

    auto& out = channel.player;
    out = {};
    out.character = ringpass::CharacterId::Unknown;
    out.action = -1;
    out.animation = -1;
    out.rings = Rings;

    taskwk* twp = playertwp[0];
    playerwk* pwp = playerpwp[0];
    motionwk2* mwp = playermwp[0];

    if (twp)
    {
        out.character = ToRingPassCharacter(
            static_cast<int>(twp->counter.b[1]));

        out.position = {
            twp->pos.x,
            twp->pos.y,
            twp->pos.z
        };

        out.rotation = {
            static_cast<float>(twp->ang.x),
            static_cast<float>(twp->ang.y),
            static_cast<float>(twp->ang.z)
        };

        out.action = static_cast<std::int32_t>(twp->mode);
        out.grounded = (twp->flag & 3) ? 1 : 0;
    }
    else
    {
        out.character = ToRingPassCharacter(
            static_cast<int>(CurrentCharacter));
    }

    if (pwp)
        out.animation = static_cast<std::int32_t>(pwp->mj.reqaction);

    if (mwp)
        out.velocity = {
            mwp->spd.x,
            mwp->spd.y,
            mwp->spd.z
        };
    else if (pwp)
        out.velocity = {
            pwp->spd.x,
            pwp->spd.y,
            pwp->spd.z
        };

    channel.protocolVersion = ringpass::kProtocolVersion;
    channel.frame = ++g_frame;
    channel.heartbeatMs = GetTickCount64();

    ringpass::end_write(channel);
}

void PrepareHostTargets()
{
    if (!g_ipc.get())
        return;

    ringpass::ErToSadxChannel host{};

    if (!ringpass::read_stable(g_ipc.get()->er, host))
        return;

    if (ringpass::channel_health(
            host.protocolVersion,
            host.heartbeatMs) != ringpass::ChannelHealth::Live)
    {
        if (g_lastPreparedTargetCount != 0)
        {
            g_targetAdapter.rebuild({}, nullptr);
            g_lastPreparedTargetCount = 0;
            Log("ER channel not live; cleared prepared host targets");
        }
        return;
    }

    g_targetAdapter.rebuild(host, playertwp[0]);

    if (g_targetAdapter.count() != g_lastPreparedTargetCount)
    {
        g_lastPreparedTargetCount = g_targetAdapter.count();
        Log(
            "prepared " +
            std::to_string(g_lastPreparedTargetCount) +
            " host target proxies (not injected into SADX list yet)");
    }
}

} // namespace

extern "C"
{
    __declspec(dllexport) void __cdecl Init(
        const char* path,
        const HelperFunctions& helperFunctions)
    {
        (void)helperFunctions;

        std::filesystem::path logPath;

        if (path && *path)
            logPath = std::filesystem::path(path) /
                      "logs" /
                      "ringpass-sadx.log";
        else
            logPath = std::filesystem::temp_directory_path() /
                      "RingPass" /
                      "ringpass-sadx.log";

        g_logger = std::make_unique<ringpass::Logger>(logPath);

        if (g_ipc.open_or_create())
            Log("RingPassSADX initialized; IPC v4 ready");
        else
            Log("RingPassSADX initialized; IPC open/create failed");

        OutputDebugStringA(
            "[RingPass-SADX] IPC v4 initialized.\n");
    }

    __declspec(dllexport) void __cdecl OnFrame()
    {
        PublishPlayerState();
        PrepareHostTargets();
    }

    __declspec(dllexport) ModInfo SADXModInfo { ModLoaderVer };
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        g_ipc.close();
    }

    return TRUE;
}
