#include "host_environment.hpp"

#include <sstream>

namespace ringpass::er {

std::filesystem::path HostEnvironment::log_path()
{
    wchar_t buffer[MAX_PATH]{};
    const DWORD count = GetEnvironmentVariableW(
        L"LOCALAPPDATA",
        buffer,
        static_cast<DWORD>(std::size(buffer)));

    std::filesystem::path root;

    if (count > 0 && count < std::size(buffer))
        root = buffer;
    else
        root = std::filesystem::temp_directory_path();

    return root / "RingPass" / "logs" / "ringpass-er.log";
}

HostEnvironment::HostEnvironment()
    : logger_(log_path())
{
}

bool HostEnvironment::initialize()
{
    HMODULE game = GetModuleHandleW(L"eldenring.exe");

    if (!game)
    {
        logger_.write("eldenring.exe module not found");
        ready_ = false;
        return false;
    }

    if (!scanner_.initialize(game))
    {
        logger_.write("failed to parse Elden Ring PE image");
        ready_ = false;
        return false;
    }

    const auto& fp = scanner_.fingerprint();

    std::ostringstream line;
    line << "host initialized; PE timestamp=0x"
         << std::hex << fp.peTimestamp
         << " imageSize=0x" << fp.imageSize
         << " checksum=0x" << fp.checksum
         << " executableRegions=" << std::dec
         << scanner_.regions().size();

    logger_.write(line.str());
    ready_ = true;
    return true;
}

} // namespace ringpass::er
