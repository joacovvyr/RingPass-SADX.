#include <windows.h>
#include <tlhelp32.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

#include <ringpass/ipc.hpp>

namespace {

bool ProcessExists(const wchar_t* name)
{
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    bool found = false;
    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(entry.szExeFile, name) == 0)
            {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return found;
}

bool Fresh(std::uint64_t heartbeat)
{
    if (!heartbeat)
        return false;

    const std::uint64_t now = GetTickCount64();
    return now >= heartbeat &&
           (now - heartbeat) <= ringpass::kHeartbeatTimeoutMs;
}

const char* YesNo(bool value)
{
    return value ? "YES" : "NO";
}

} // namespace

int main()
{
    while (true)
    {
        const bool sadxProcess =
            ProcessExists(L"sonic.exe") ||
            ProcessExists(L"Sonic Adventure DX.exe");
        const bool erProcess = ProcessExists(L"eldenring.exe");

        ringpass::SharedMemory ipc;
        const bool ipcOpen = ipc.open_existing(FILE_MAP_READ);

        bool sadxChannel = false;
        bool erChannel = false;

        if (ipcOpen)
        {
            ringpass::SadxToErChannel sadx{};
            ringpass::ErToSadxChannel er{};

            sadxChannel =
                ringpass::read_stable(ipc.get()->sadx, sadx) &&
                sadx.protocolVersion == ringpass::kProtocolVersion &&
                Fresh(sadx.heartbeatMs);

            erChannel =
                ringpass::read_stable(ipc.get()->er, er) &&
                er.protocolVersion == ringpass::kProtocolVersion &&
                Fresh(er.heartbeatMs);
        }

        std::cout << "\x1b[2J\x1b[H";
        std::cout << "RingPass Launcher / Diagnostics 0.3\n\n";
        std::cout << "SADX process:       " << YesNo(sadxProcess) << "\n";
        std::cout << "Elden Ring process: " << YesNo(erProcess) << "\n";
        std::cout << "IPC v3:             " << YesNo(ipcOpen) << "\n";
        std::cout << "SADX bridge live:   " << YesNo(sadxChannel) << "\n";
        std::cout << "ER bridge live:     " << YesNo(erChannel) << "\n\n";

        if (sadxChannel && erChannel)
            std::cout << "BRIDGE READY\n";
        else
            std::cout << "Waiting for live bridge heartbeats...\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
