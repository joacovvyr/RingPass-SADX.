#include <windows.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>

#include "SADXModLoader.h"
#include "settings.hpp"
#include "target_injection.hpp"
#include "target_proxy_adapter.hpp"

#include <ringpass/coordinates.hpp>
#include <ringpass/health.hpp>
#include <ringpass/ipc.hpp>
#include <ringpass/logger.hpp>

namespace {

ringpass::SharedMemory g_ipc;
std::uint64_t g_frame = 0;

std::unique_ptr<ringpass::Logger> g_logger;
ringpass::sadx::Settings g_settings{};
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

        out.action =
            static_cast<std::int32_t>(twp->mode);

        out.grounded =
            (twp->flag & 3) ? 1 : 0;
    }
    else
    {
        out.character = ToRingPassCharacter(
            static_cast<int>(CurrentCharacter));
    }

    if (pwp)
        out.animation =
            static_cast<std::int32_t>(
                pwp->mj.reqaction);

    if (mwp)
    {
        out.velocity = {
            mwp->spd.x,
            mwp->spd.y,
            mwp->spd.z
        };
    }
    else if (pwp)
    {
        out.velocity = {
            pwp->spd.x,
            pwp->spd.y,
            pwp->spd.z
        };
    }

    channel.protocolVersion =
        ringpass::kProtocolVersion;

    channel.frame = ++g_frame;
    channel.heartbeatMs = GetTickCount64();

    ringpass::end_write(channel);
}

void ClearPreparedTargets(
    const char* reason)
{
    if (g_lastPreparedTargetCount != 0)
    {
        g_targetAdapter.clear();
        g_lastPreparedTargetCount = 0;

        if (reason)
            Log(reason);
    }
}

void PrepareHostTargets()
{
    if (!g_ipc.get())
        return;

    taskwk* player = playertwp[0];

    if (!player)
    {
        ClearPreparedTargets(
            "SADX player unavailable; cleared prepared host targets");
        return;
    }

    ringpass::ErToSadxChannel host{};

    if (!ringpass::read_stable(
            g_ipc.get()->er,
            host))
        return;

    if (ringpass::channel_health(
            host.protocolVersion,
            host.heartbeatMs) !=
        ringpass::ChannelHealth::Live)
    {
        ClearPreparedTargets(
            "ER channel not live; cleared prepared host targets");
        return;
    }

    ringpass::CoordinateTransform transform{};

    // CoordinateTransform::er_to_sadx divides the host delta by
    // scale, so this setting means exactly "ER units per SADX unit".
    transform.scale =
        g_settings.erUnitsPerSadxUnit;

    transform.sadxOrigin =
    {
        player->pos.x,
        player->pos.y,
        player->pos.z
    };

    transform.erOrigin =
        host.hostPlayerPosition;

    transform.swapYZ =
        g_settings.swapYZ;

    transform.invertX =
        g_settings.invertX;

    transform.invertZ =
        g_settings.invertZ;

    g_targetAdapter.rebuild(
        host,
        player,
        transform);

    if (g_targetAdapter.count() !=
        g_lastPreparedTargetCount)
    {
        g_lastPreparedTargetCount =
            g_targetAdapter.count();

        Log(
            "prepared " +
            std::to_string(
                g_lastPreparedTargetCount) +
            " mapped host target proxies" +
            (g_settings.injectTargets
                ? " for native SADX targeting"
                : " (native injection disabled)"));
    }
}

void LogSettings()
{
    std::ostringstream line;

    line << "settings: InjectTargets="
         << (g_settings.injectTargets ? 1 : 0)
         << " ERUnitsPerSADXUnit="
         << g_settings.erUnitsPerSadxUnit
         << " SwapYZ="
         << (g_settings.swapYZ ? 1 : 0)
         << " InvertX="
         << (g_settings.invertX ? 1 : 0)
         << " InvertZ="
         << (g_settings.invertZ ? 1 : 0);

    Log(line.str());
}

} // namespace

extern "C"
{
    __declspec(dllexport)
    void __cdecl Init(
        const char* path,
        const HelperFunctions& helperFunctions)
    {
        (void)helperFunctions;

        const std::filesystem::path modPath =
            (path && *path)
                ? std::filesystem::path(path)
                : std::filesystem::temp_directory_path() /
                    "RingPass";

        const auto logPath =
            modPath /
            "logs" /
            "ringpass-sadx.log";

        g_logger =
            std::make_unique<ringpass::Logger>(
                logPath);

        g_settings =
            ringpass::sadx::load_settings(
                modPath);

        LogSettings();

        if (g_ipc.open_or_create())
            Log("RingPassSADX initialized; IPC v5 ready");
        else
            Log("RingPassSADX initialized; IPC open/create failed");

        if (g_settings.injectTargets)
        {
            if (!ringpass::sadx::initialize_target_injection(
                    &g_targetAdapter,
                    g_logger.get()))
            {
                Log(
                    "experimental target injection requested "
                    "but hook initialization failed");
            }
        }
        else
        {
            Log(
                "experimental native target injection disabled "
                "(safe default)");
        }

        OutputDebugStringA(
            "[RingPass-SADX] IPC v5 initialized.\n");
    }

    __declspec(dllexport)
    void __cdecl OnFrame()
    {
        PublishPlayerState();
        PrepareHostTargets();
    }

    __declspec(dllexport)
    ModInfo SADXModInfo { ModLoaderVer };
}

BOOL APIENTRY DllMain(
    HMODULE module,
    DWORD reason,
    LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        ringpass::sadx::shutdown_target_injection();
        g_ipc.close();
    }

    return TRUE;
}
