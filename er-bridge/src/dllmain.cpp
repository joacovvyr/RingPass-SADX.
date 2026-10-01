#include <windows.h>

#include <atomic>
#include <cstdint>

#include "host_environment.hpp"
#include <ringpass/ipc.hpp>

namespace {

ringpass::SharedMemory g_ipc;
std::atomic_bool g_running{false};
HANDLE g_thread = nullptr;
std::uint64_t g_frame = 0;
ringpass::er::HostEnvironment g_host;

void PublishHostState(ringpass::HostState hostState)
{
    if (!g_ipc.open_or_create())
        return;

    auto& channel = g_ipc.get()->er;
    ringpass::begin_write(channel);
    channel.protocolVersion = ringpass::kProtocolVersion;
    channel.hostState = hostState;
    channel.frame = ++g_frame;
    channel.heartbeatMs = GetTickCount64();
    ringpass::end_write(channel);
}

DWORD WINAPI BridgeThread(LPVOID)
{
    const bool hostReady = g_host.initialize();

    if (!g_ipc.open_or_create())
    {
        g_host.log().write("failed to open RingPass IPC");
        return 1;
    }

    if (!hostReady)
    {
        PublishHostState(ringpass::HostState::Offline);
        g_host.log().write("bridge disabled because host validation failed");
        return 2;
    }

    g_host.log().write("RingPassER bridge thread started");
    PublishHostState(ringpass::HostState::Booting);

    while (g_running.load())
    {
        // Build-specific signatures will be registered here.
        // Until a signature set has been validated on the user's executable,
        // publish only liveness and host fingerprint information.
        PublishHostState(ringpass::HostState::Booting);
        Sleep(250);
    }

    PublishHostState(ringpass::HostState::Offline);
    g_host.log().write("RingPassER bridge thread stopped");
    return 0;
}

} // namespace

extern "C" __declspec(dllexport) std::uint32_t RingPassER_ProtocolVersion()
{
    return ringpass::kProtocolVersion;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        g_running.store(true);
        g_thread = CreateThread(nullptr, 0, BridgeThread, nullptr, 0, nullptr);
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
