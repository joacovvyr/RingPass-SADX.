#include <windows.h>
#include <tlhelp32.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <deque>
#include <iostream>
#include <string>

#include <ringpass/health.hpp>
#include <ringpass/ipc.hpp>

namespace {

DWORD process_id(const wchar_t* name)
{
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    DWORD pid = 0;
    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(entry.szExeFile, name) == 0)
            {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pid;
}

std::filesystem::path process_path(DWORD pid)
{
    if (!pid)
        return {};

    HANDLE process = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION,
        FALSE,
        pid);

    if (!process)
        return {};

    wchar_t buffer[32768]{};
    DWORD size = static_cast<DWORD>(std::size(buffer));

    std::filesystem::path result;

    if (QueryFullProcessImageNameW(
            process,
            0,
            buffer,
            &size))
    {
        result.assign(buffer, buffer + size);
    }

    CloseHandle(process);
    return result;
}

std::string file_version(const std::filesystem::path& path)
{
    if (path.empty())
        return "unknown";

    DWORD ignored = 0;
    const DWORD size = GetFileVersionInfoSizeW(
        path.c_str(),
        &ignored);

    if (!size)
        return "unknown";

    std::vector<std::byte> data(size);

    if (!GetFileVersionInfoW(
            path.c_str(),
            0,
            size,
            data.data()))
        return "unknown";

    VS_FIXEDFILEINFO* info = nullptr;
    UINT infoSize = 0;

    if (!VerQueryValueW(
            data.data(),
            L"\\",
            reinterpret_cast<void**>(&info),
            &infoSize) ||
        !info ||
        infoSize < sizeof(VS_FIXEDFILEINFO))
        return "unknown";

    std::ostringstream out;
    out << HIWORD(info->dwFileVersionMS) << '.'
        << LOWORD(info->dwFileVersionMS) << '.'
        << HIWORD(info->dwFileVersionLS) << '.'
        << LOWORD(info->dwFileVersionLS);

    return out.str();
}

std::filesystem::path exe_dir()
{
    wchar_t buffer[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}


std::filesystem::path er_log_path()
{
    wchar_t buffer[MAX_PATH]{};
    const DWORD count = GetEnvironmentVariableW(
        L"LOCALAPPDATA",
        buffer,
        MAX_PATH);

    std::filesystem::path root;
    if (count > 0 && count < MAX_PATH)
        root = buffer;
    else
        root = std::filesystem::temp_directory_path();

    return root / "RingPass" / "logs" / "ringpass-er.log";
}

void append_log_tail(
    std::ofstream& report,
    const std::filesystem::path& path,
    std::size_t maxLines = 40)
{
    report << "\n[ER log tail]\n";
    report << "path=" << path.string() << "\n";

    std::ifstream in(path);
    if (!in)
    {
        report << "(log file not found)\n";
        return;
    }

    std::deque<std::string> lines;
    std::string line;

    while (std::getline(in, line))
    {
        lines.push_back(line);
        if (lines.size() > maxLines)
            lines.pop_front();
    }

    for (const auto& value : lines)
        report << value << "\n";
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

    DWORD sadxPid = process_id(L"sonic.exe");
    if (!sadxPid)
        sadxPid = process_id(L"Sonic Adventure DX.exe");

    const DWORD erPid = process_id(L"eldenring.exe");

    const bool sadxProcess = sadxPid != 0;
    const bool erProcess = erPid != 0;

    const auto erPath = process_path(erPid);
    const auto erVersion = file_version(erPath);

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
    report << "Elden Ring: " << bool_name(erProcess) << "\n";
    report << "Elden Ring PID: " << erPid << "\n";
    report << "Elden Ring path: " << erPath.string() << "\n";
    report << "Elden Ring file version: " << erVersion << "\n\n";

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

        const std::uint32_t shown =
            er.targetCount < 3 ? er.targetCount : 3;

        for (std::uint32_t i = 0; i < shown; ++i)
        {
            const auto& t = er.targets[i];

            report << "target[" << i << "].id=" << t.id << "\n";
            report << "target[" << i << "].position="
                   << t.position.x << ","
                   << t.position.y << ","
                   << t.position.z << "\n";
            report << "target[" << i << "].aimPoint="
                   << t.aimPoint.x << ","
                   << t.aimPoint.y << ","
                   << t.aimPoint.z << "\n";
            report << "target[" << i << "].radius="
                   << t.radius << "\n";
            report << "target[" << i << "].hp="
                   << t.hp << "\n";
            report << "target[" << i << "].targetable="
                   << static_cast<int>(t.targetable) << "\n";
            report << "target[" << i << "].alive="
                   << static_cast<int>(t.alive) << "\n";
        }
    }

    append_log_tail(report, er_log_path());

    report.flush();

    std::cout << "RingPass Doctor complete.\n";
    std::cout << "Report: " << reportPath.string() << "\n";
    std::cout << "SADX bridge: "
              << ringpass::channel_health_name(sadxHealth) << "\n";
    std::cout << "ER bridge: "
              << ringpass::channel_health_name(erHealth) << "\n";

    return 0;
}
