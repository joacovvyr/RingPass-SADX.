#include <windows.h>
#include <tlhelp32.h>

#include <chrono>
#include <iostream>
#include <thread>

#include <ringpass/health.hpp>
#include <ringpass/ipc.hpp>

namespace {

bool ProcessExists(const wchar_t* name)
{
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    HANDLE snapshot =
        CreateToolhelp32Snapshot(
            TH32CS_SNAPPROCESS,
            0);

    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    bool found = false;

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(
                    entry.szExeFile,
                    name) == 0)
            {
                found = true;
                break;
            }
        }
        while (Process32NextW(
            snapshot,
            &entry));
    }

    CloseHandle(snapshot);
    return found;
}

const char* YesNo(bool value)
{
    return value ? "YES" : "NO";
}

const char* HostName(ringpass::HostState state)
{
    switch (state)
    {
    case ringpass::HostState::Booting:
        return "BOOTING";
    case ringpass::HostState::Menu:
        return "MENU";
    case ringpass::HostState::InWorld:
        return "IN WORLD";
    default:
        return "OFFLINE";
    }
}

} // namespace

int main()
{
    while (true)
    {
        const bool sadxProcess =
            ProcessExists(L"sonic.exe") ||
            ProcessExists(
                L"Sonic Adventure DX.exe");

        const bool erProcess =
            ProcessExists(L"eldenring.exe");

        ringpass::SharedMemory ipc;
        const bool ipcOpen =
            ipc.open_existing(FILE_MAP_READ);

        auto sadxHealth =
            ringpass::ChannelHealth::Offline;

        auto erHealth =
            ringpass::ChannelHealth::Offline;

        ringpass::HostState hostState =
            ringpass::HostState::Offline;

        if (ipcOpen)
        {
            ringpass::SadxToErChannel sadx{};
            ringpass::ErToSadxChannel er{};

            if (ringpass::read_stable(
                    ipc.get()->sadx,
                    sadx))
            {
                sadxHealth =
                    ringpass::channel_health(
                        sadx.protocolVersion,
                        sadx.heartbeatMs);
            }

            if (ringpass::read_stable(
                    ipc.get()->er,
                    er))
            {
                erHealth =
                    ringpass::channel_health(
                        er.protocolVersion,
                        er.heartbeatMs);

                hostState = er.hostState;
            }
        }

        std::cout << "\x1b[2J\x1b[H";
        std::cout
            << "RingPass Launcher / Diagnostics 0.4\n\n";

        std::cout << "SADX process:       "
                  << YesNo(sadxProcess)
                  << "\n";

        std::cout << "Elden Ring process: "
                  << YesNo(erProcess)
                  << "\n";

        std::cout << "IPC v"
                  << ringpass::kProtocolVersion
                  << ":             "
                  << YesNo(ipcOpen)
                  << "\n";

        std::cout << "SADX bridge:        "
                  << ringpass::channel_health_name(
                      sadxHealth)
                  << "\n";

        std::cout << "ER bridge:          "
                  << ringpass::channel_health_name(
                      erHealth)
                  << "\n";

        std::cout << "ER state:           "
                  << HostName(hostState)
                  << "\n\n";

        if (sadxHealth ==
                ringpass::ChannelHealth::Live &&
            erHealth ==
                ringpass::ChannelHealth::Live)
        {
            std::cout << "BRIDGE READY\n";
        }
        else
        {
            std::cout
                << "Waiting for both live bridges...\n";
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(500));
    }
}
