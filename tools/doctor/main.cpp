#include <windows.h>
#include <tlhelp32.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <ringpass/health.hpp>
#include <ringpass/ipc.hpp>

namespace {

bool process_exists(const wchar_t* name)
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

std::filesystem::path exe_dir()
{
    wchar_t buffer[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}

const char* bool_name(bool value)
{
    return value ? "YES" : "NO";
}

} // namespace

int main()
{
    const auto dir = exe_dir();
    const auto reportPath = dir / "RingPass-Diagnostic.txt";

    std::ofstream report(reportPath, std::ios::trunc);
    if (!report)
    {
        std::cerr << "Could not create diagnostic report.\n";
        return 1;
    }

    const bool sadxProcess =
        process_exists(L"sonic.exe") ||
        process_exists(L"Sonic Adventure DX.exe");
    const bool erProcess = process_exists(L"eldenring.exe");

    const bool hasMonitor =
        std::filesystem::exists(dir / "RingPassMonitor.exe");
    const bool hasSelfTest =
        std::filesystem::exists(dir / "RingPassSelfTest.exe");
    const bool hasFakeSadx =
        std::filesystem::exists(dir / "FakeSADX.exe");
    const bool hasFakeEr =
        std::filesystem::exists(dir / "FakeER.exe");

    ringpass::SharedMemory ipc;
    const bool ipcOpen = ipc.open_existing(FILE_MAP_READ);

    auto sadxHealth = ringpass::ChannelHealth::Offline;
    auto erHealth = ringpass::ChannelHealth::Offline;

    ringpass::SadxToErChannel sadx{};
    ringpass::ErToSadxChannel er{};

    if (ipcOpen)
    {
        if (ringpass::read_stable(ipc.get()->sadx, sadx))
            sadxHealth = ringpass::channel_health(
                sadx.protocolVersion,
                sadx.heartbeatMs);

        if (ringpass::read_stable(ipc.get()->er, er))
            erHealth = ringpass::channel_health(
                er.protocolVersion,
                er.heartbeatMs);
    }

    report << "RingPass Diagnostic Report\n";
    report << "Protocol: " << ringpass::kProtocolVersion << "\n\n";

    report << "[Processes]\n";
    report << "SADX: " << bool_name(sadxProcess) << "\n";
    report << "Elden Ring: " << bool_name(erProcess) << "\n\n";

    report << "[Tools]\n";
    report << "Monitor: " << bool_name(hasMonitor) << "\n";
    report << "SelfTest: " << bool_name(hasSelfTest) << "\n";
    report << "FakeSADX: " << bool_name(hasFakeSadx) << "\n";
    report << "FakeER: " << bool_name(hasFakeEr) << "\n\n";

    report << "[IPC]\n";
    report << "Open: " << bool_name(ipcOpen) << "\n";
    report << "SADX channel: "
           << ringpass::channel_health_name(sadxHealth) << "\n";
    report << "ER channel: "
           << ringpass::channel_health_name(erHealth) << "\n";

    if (ipcOpen)
    {
        report << "\n[SADX frame]\n";
        report << "frame=" << sadx.frame << "\n";
        report << "character="
               << static_cast<std::uint32_t>(sadx.player.character) << "\n";
        report << "position="
               << sadx.player.position.x << ","
               << sadx.player.position.y << ","
               << sadx.player.position.z << "\n";
        report << "velocity="
               << sadx.player.velocity.x << ","
               << sadx.player.velocity.y << ","
               << sadx.player.velocity.z << "\n";
        report << "action=" << sadx.player.action << "\n";
        report << "animation=" << sadx.player.animation << "\n";
        report << "rings=" << sadx.player.rings << "\n";
        report << "grounded=" << static_cast<int>(sadx.player.grounded) << "\n";

        report << "\n[ER frame]\n";
        report << "frame=" << er.frame << "\n";
        report << "hostState=" << static_cast<std::uint32_t>(er.hostState) << "\n";
        report << "hostZone=" << er.hostZone << "\n";
        report << "playerPosition="
               << er.hostPlayerPosition.x << ","
               << er.hostPlayerPosition.y << ","
               << er.hostPlayerPosition.z << "\n";
        report << "cameraPosition="
               << er.camera.position.x << ","
               << er.camera.position.y << ","
               << er.camera.position.z << "\n";
        report << "cameraFovRadians=" << er.camera.fovRadians << "\n";
        report << "groundHit=" << static_cast<int>(er.groundProbe.hit) << "\n";
        report << "groundDistance=" << er.groundProbe.distance << "\n";
        report << "targetCount=" << er.targetCount << "\n";
    }

    report.flush();

    std::cout << "RingPass Doctor complete.\n";
    std::cout << "Report: " << reportPath.string() << "\n";
    std::cout << "SADX bridge: "
              << ringpass::channel_health_name(sadxHealth) << "\n";
    std::cout << "ER bridge: "
              << ringpass::channel_health_name(erHealth) << "\n";

    return 0;
}
