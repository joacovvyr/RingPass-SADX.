#include <windows.h>
#include <cstdint>

#include "SADXModLoader.h"
#include <ringpass/ipc.hpp>

namespace {

ringpass::SharedMemory g_ipc;
std::uint64_t g_frame = 0;

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
        out.character = ToRingPassCharacter(static_cast<int>(twp->counter.b[1]));
        out.position = { twp->pos.x, twp->pos.y, twp->pos.z };
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
        out.character = ToRingPassCharacter(static_cast<int>(CurrentCharacter));
    }

    if (pwp)
        out.animation = static_cast<std::int32_t>(pwp->mj.reqaction);

    if (mwp)
        out.velocity = { mwp->spd.x, mwp->spd.y, mwp->spd.z };
    else if (pwp)
        out.velocity = { pwp->spd.x, pwp->spd.y, pwp->spd.z };

    channel.protocolVersion = ringpass::kProtocolVersion;
    channel.frame = ++g_frame;
    channel.heartbeatMs = GetTickCount64();

    ringpass::end_write(channel);
}

} // namespace

extern "C"
{
    __declspec(dllexport) void __cdecl Init(
        const char* path,
        const HelperFunctions& helperFunctions)
    {
        (void)path;
        (void)helperFunctions;

        g_ipc.open_or_create();
        OutputDebugStringA("[RingPass-SADX] IPC v3 initialized.\n");
    }

    __declspec(dllexport) void __cdecl OnFrame()
    {
        PublishPlayerState();
    }

    __declspec(dllexport) ModInfo SADXModInfo { ModLoaderVer };
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(module);
    else if (reason == DLL_PROCESS_DETACH)
        g_ipc.close();

    return TRUE;
}
