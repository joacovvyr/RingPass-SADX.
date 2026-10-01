#include <windows.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

#include <ringpass/health.hpp>
#include <ringpass/ipc.hpp>

namespace {

struct ChildProcess {
    PROCESS_INFORMATION pi{};

    void stop()
    {
        if (pi.hProcess)
        {
            TerminateProcess(pi.hProcess, 0);
            WaitForSingleObject(pi.hProcess, 2000);
            CloseHandle(pi.hProcess);
            pi.hProcess = nullptr;
        }

        if (pi.hThread)
        {
            CloseHandle(pi.hThread);
            pi.hThread = nullptr;
        }
    }

    ~ChildProcess() { stop(); }
};

bool start_child(
    const std::filesystem::path& exe,
    ChildProcess& child)
{
    STARTUPINFOW si{};
    si.cb = sizeof(si);

    std::wstring command = L"\"" + exe.wstring() + L"\"";
    std::vector<wchar_t> buffer(command.begin(), command.end());
    buffer.push_back(L'\0');

    return CreateProcessW(
        exe.c_str(),
        buffer.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_CONSOLE,
        nullptr,
        exe.parent_path().c_str(),
        &si,
        &child.pi) != FALSE;
}

std::filesystem::path executable_dir()
{
    wchar_t buffer[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}

} // namespace

int main()
{
    std::cout << "RingPass Self Test 0.4\n\n";

    const auto dir = executable_dir();
    const auto fakeSadx = dir / "FakeSADX.exe";
    const auto fakeEr = dir / "FakeER.exe";

    if (!std::filesystem::exists(fakeSadx) ||
        !std::filesystem::exists(fakeEr))
    {
        std::cerr << "FAIL: FakeSADX.exe and FakeER.exe must be next to this tool.\n";
        return 1;
    }

    ChildProcess sadx;
    ChildProcess er;

    if (!start_child(fakeSadx, sadx))
    {
        std::cerr << "FAIL: could not start FakeSADX.exe\n";
        return 2;
    }

    if (!start_child(fakeEr, er))
    {
        std::cerr << "FAIL: could not start FakeER.exe\n";
        return 3;
    }

    ringpass::SharedMemory ipc;

    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(8);

    bool passed = false;

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (!ipc.open_existing(FILE_MAP_READ))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        ringpass::SadxToErChannel sadxFrame{};
        ringpass::ErToSadxChannel erFrame{};

        const bool sadxRead =
            ringpass::read_stable(ipc.get()->sadx, sadxFrame);
        const bool erRead =
            ringpass::read_stable(ipc.get()->er, erFrame);

        const bool sadxLive =
            sadxRead &&
            ringpass::channel_health(
                sadxFrame.protocolVersion,
                sadxFrame.heartbeatMs) ==
                ringpass::ChannelHealth::Live;

        const bool erLive =
            erRead &&
            ringpass::channel_health(
                erFrame.protocolVersion,
                erFrame.heartbeatMs) ==
                ringpass::ChannelHealth::Live;

        if (sadxLive &&
            erLive &&
            sadxFrame.frame > 10 &&
            erFrame.frame > 10 &&
            erFrame.hostState == ringpass::HostState::InWorld &&
            erFrame.groundProbe.hit &&
            erFrame.targetCount == 3)
        {
            passed = true;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    sadx.stop();
    er.stop();

    if (!passed)
    {
        std::cerr << "FAIL: IPC simulation did not reach the expected state.\n";
        return 4;
    }

    std::cout << "PASS\n";
    std::cout << "- SADX -> ER channel live\n";
    std::cout << "- ER -> SADX channel live\n";
    std::cout << "- protocol compatible\n";
    std::cout << "- ground probe received\n";
    std::cout << "- 3 target proxies received\n";
    return 0;
}
