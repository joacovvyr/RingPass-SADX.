#include <windows.h>

#include <atomic>
#include <cstdint>

#include "host_environment.hpp"
#include "world_reader.hpp"

#include <ringpass/ipc.hpp>

namespace {

ringpass::SharedMemory g_ipc;
std::atomic_bool g_running{false};
HANDLE g_thread = nullptr;
std::uint64_t g_frame = 0;

ringpass::er::HostEnvironment g_host;
ringpass::er::WorldReader g_world;

void PublishHostSample(
    ringpass::HostState hostState,
    const ringpass::er::HostSample* sample)
{
    if (!g_ipc.open_or_create())
        return;

    auto& channel = g_ipc.get()->er;
    ringpass::begin_write(channel);

    channel.protocolVersion = ringpass::kProtocolVersion;
    channel.hostState = hostState;
    channel.frame = ++g_frame;
    channel.heartbeatMs = GetTickCount64();

    if (sample)
    {
        if (sample->playerValid)
            channel.hostPlayerPosition =
                sample->playerPosition;

        if (sample->cameraValid)
            channel.camera = sample->camera;
    }

    ringpass::end_write(channel);
}

DWORD WINAPI BridgeThread(LPVOID)
{
    if (!g_host.initialize())
        return 1;

    if (!g_ipc.open_or_create())
    {
        g_host.log().write(
            "failed to open RingPass IPC");
        return 2;
    }

    const bool worldReaderReady =
        g_world.initialize(
            g_host.scanner(),
            g_host.log());

    if (!worldReaderReady)
    {
        g_host.log().write(
            "world reader signatures unavailable; "
            "continuing with liveness-only mode");
    }
    else
    {
        g_host.log().write(
            "read-only player/camera reader ready");
    }

    g_host.log().write(
        "RingPassER bridge thread started");

    while (g_running.load())
    {
        ringpass::er::HostSample sample{};

        if (worldReaderReady &&
            g_world.sample(sample))
        {
            const auto state =
                sample.playerValid
                    ? ringpass::HostState::InWorld
                    : ringpass::HostState::Menu;

            PublishHostSample(state, &sample);
        }
        else
        {
            PublishHostSample(
                ringpass::HostState::Booting,
                nullptr);
        }

        Sleep(16);
    }

    PublishHostSample(
        ringpass::HostState::Offline,
        nullptr);

    g_host.log().write(
        "RingPassER bridge thread stopped");

    return 0;
}

} // namespace

extern "C" __declspec(dllexport)
std::uint32_t RingPassER_ProtocolVersion()
{
    return ringpass::kProtocolVersion;
}

BOOL APIENTRY DllMain(
    HMODULE module,
    DWORD reason,
    LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        g_running.store(true);
        g_thread = CreateThread(
            nullptr,
            0,
            BridgeThread,
            nullptr,
            0,
            nullptr);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        g_running.store(false);

        if (g_thread)
        {
            CloseHandle(g_thread);
            g_thread = nullptr;
        }

        g_ipc.close();
    }

    return TRUE;
}
