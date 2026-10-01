#include <windows.h>
#include <cstdint>
#include <cstring>

#include "SADXModLoader.h"
#include <ringpass/protocol.hpp>

namespace {

constexpr const char* kMappingName = "Local\\RingPassSADX_SharedFrame_v1";

HANDLE g_mapping = nullptr;
ringpass::SharedFrame* g_shared = nullptr;
std::uint64_t g_frame = 0;

ringpass::CharacterId ToRingPassCharacter(int sadxCharacter)
{
    switch (sadxCharacter)
    {
    case Characters_Sonic:
        return ringpass::CharacterId::Sonic;
    case Characters_Tails:
        return ringpass::CharacterId::Tails;
    case Characters_Knuckles:
        return ringpass::CharacterId::Knuckles;
    case Characters_Amy:
        return ringpass::CharacterId::Amy;
    case Characters_Gamma:
        return ringpass::CharacterId::Gamma;
    case Characters_Big:
        return ringpass::CharacterId::Big;
    default:
        return ringpass::CharacterId::Unknown;
    }
}

void OpenSharedMemory()
{
    if (g_shared)
        return;

    g_mapping = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        static_cast<DWORD>(sizeof(ringpass::SharedFrame)),
        kMappingName);

    if (!g_mapping)
        return;

    g_shared = static_cast<ringpass::SharedFrame*>(
        MapViewOfFile(
            g_mapping,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            sizeof(ringpass::SharedFrame)));

    if (!g_shared)
    {
        CloseHandle(g_mapping);
        g_mapping = nullptr;
        return;
    }

    std::memset(g_shared, 0, sizeof(ringpass::SharedFrame));
    g_shared->protocolVersion = ringpass::kProtocolVersion;
    g_shared->player.character = ringpass::CharacterId::Unknown;
}

void CloseSharedMemory()
{
    if (g_shared)
    {
        UnmapViewOfFile(g_shared);
        g_shared = nullptr;
    }

    if (g_mapping)
    {
        CloseHandle(g_mapping);
        g_mapping = nullptr;
    }
}

void PublishPlayerState()
{
    if (!g_shared)
        return;

    // Odd sequence = writer is updating. Even sequence = stable snapshot.
    ++g_shared->sequence;

    auto& out = g_shared->player;
    out.protocolVersion = ringpass::kProtocolVersion;
    out.character = ringpass::CharacterId::Unknown;
    out.position = {};
    out.velocity = {};
    out.rotation = {};
    out.action = -1;
    out.animation = -1;
    out.rings = Rings;
    out.grounded = 0;

    taskwk* twp = playertwp[0];
    playerwk* pwp = playerpwp[0];
    motionwk2* mwp = playermwp[0];

    if (twp)
    {
        out.character = ToRingPassCharacter(static_cast<int>(twp->counter.b[1]));

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
        // CurrentCharacter remains useful while transitioning between game states.
        out.character = ToRingPassCharacter(static_cast<int>(CurrentCharacter));
    }

    if (pwp)
    {
        out.animation = static_cast<std::int32_t>(pwp->mj.reqaction);
    }

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

    g_shared->sadxFrame = ++g_frame;

    MemoryBarrier();
    ++g_shared->sequence;
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
        OpenSharedMemory();
        OutputDebugStringA("[RingPass-SADX] Bridge initialized.\n");
    }

    __declspec(dllexport) void __cdecl OnFrame()
    {
        if (!g_shared)
            OpenSharedMemory();

        PublishPlayerState();
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
        CloseSharedMemory();
    }

    return TRUE;
}
