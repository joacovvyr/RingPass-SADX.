#include <windows.h>

#include <atomic>
#include <chrono>
#include <thread>

#include <ringpass/ipc.hpp>

namespace {

ringpass::SharedMemory g_ipc;
std::atomic_bool g_running{false};
HANDLE g_thread = nullptr;
std::uint64_t g_frame = 0;

void PublishHostState(ringpass::HostState hostState)
{
    if (!g_ipc.open_or_create())
        return;

    auto& channel = g_ipc.get()->er;
    ringpass::begin_write(channel);
    channel.protocolVersion = ringpass::kProtocolVersion;
    channel.hostState = hostState;
    channel.frame = ++g_frame;
    ringpass::end_write(channel);
}

DWORD WINAPI BridgeThread(LPVOID)
{
    g_ipc.open_or_create();
    PublishHostState(ringpass::HostState::Booting);

    while (g_running.load())
    {
        // The real Elden Ring world/camera/entity adapters plug in here.
        // Until signatures are validated against a supported game build,
        // this module deliberately publishes only bridge liveness.
        PublishHostState(ringpass::HostState::Booting);
        Sleep(250);
    }

    PublishHostState(ringpass::HostState::Offline);
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
