#include "host_environment.hpp"

#include <cstddef>
#include <sstream>
#include <vector>

namespace ringpass::er {

std::filesystem::path HostEnvironment::log_path()
{
    wchar_t buffer[MAX_PATH]{};

    const DWORD count =
        GetEnvironmentVariableW(
            L"LOCALAPPDATA",
            buffer,
            static_cast<DWORD>(std::size(buffer)));

    std::filesystem::path root;

    if (count > 0 &&
        count < std::size(buffer))
        root = buffer;
    else
        root = std::filesystem::temp_directory_path();

    return root /
           "RingPass" /
           "logs" /
           "ringpass-er.log";
}

HostEnvironment::HostEnvironment()
    : logger_(log_path())
{
}

bool HostEnvironment::read_file_version()
{
    if (!gameModule_)
        return false;

    wchar_t path[MAX_PATH]{};

    if (!GetModuleFileNameW(
            gameModule_,
            path,
            MAX_PATH))
        return false;

    DWORD ignored = 0;

    const DWORD size =
        GetFileVersionInfoSizeW(
            path,
            &ignored);

    if (!size)
        return false;

    std::vector<std::byte> data(size);

    if (!GetFileVersionInfoW(
            path,
            0,
            size,
            data.data()))
        return false;

    VS_FIXEDFILEINFO* info = nullptr;
    UINT infoSize = 0;

    if (!VerQueryValueW(
            data.data(),
            L"\\",
            reinterpret_cast<void**>(&info),
            &infoSize))
        return false;

    if (!info ||
        infoSize < sizeof(VS_FIXEDFILEINFO) ||
        info->dwSignature != 0xFEEF04BD)
        return false;

    fileVersion_[0] = HIWORD(info->dwFileVersionMS);
    fileVersion_[1] = LOWORD(info->dwFileVersionMS);
    fileVersion_[2] = HIWORD(info->dwFileVersionLS);
    fileVersion_[3] = LOWORD(info->dwFileVersionLS);

    hasFileVersion_ = true;
    return true;
}

std::string HostEnvironment::file_version_string() const
{
    if (!hasFileVersion_)
        return "unknown";

    std::ostringstream out;
    out << fileVersion_[0] << '.'
        << fileVersion_[1] << '.'
        << fileVersion_[2] << '.'
        << fileVersion_[3];

    return out.str();
}

bool HostEnvironment::initialize()
{
    gameModule_ = GetModuleHandleW(L"eldenring.exe");

    if (!gameModule_)
    {
        logger_.write("eldenring.exe module not found");
        ready_ = false;
        return false;
    }

    if (!scanner_.initialize(gameModule_))
    {
        logger_.write("failed to parse Elden Ring PE image");
        ready_ = false;
        return false;
    }

    read_file_version();

    const auto& fp = scanner_.fingerprint();

    std::ostringstream line;
    line << "host initialized; fileVersion="
         << file_version_string()
         << " PE timestamp=0x"
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
