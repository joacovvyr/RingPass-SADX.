#include "settings.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <string>

namespace ringpass::sadx {

namespace {

bool read_bool(
    const std::filesystem::path& ini,
    const char* section,
    const char* key,
    bool fallback)
{
    return GetPrivateProfileIntA(
               section,
               key,
               fallback ? 1 : 0,
               ini.string().c_str()) != 0;
}

float read_float(
    const std::filesystem::path& ini,
    const char* section,
    const char* key,
    float fallback)
{
    std::array<char, 64> buffer{};

    const std::string fallbackText =
        std::to_string(fallback);

    GetPrivateProfileStringA(
        section,
        key,
        fallbackText.c_str(),
        buffer.data(),
        static_cast<DWORD>(buffer.size()),
        ini.string().c_str());

    char* end = nullptr;
    const float value =
        std::strtof(buffer.data(), &end);

    if (end == buffer.data())
        return fallback;

    return value;
}

} // namespace

Settings load_settings(
    const std::filesystem::path& modPath)
{
    Settings out{};

    const auto ini =
        modPath / "RingPass.ini";

    out.injectTargets =
        read_bool(
            ini,
            "Experimental",
            "InjectTargets",
            false);

    out.erUnitsPerSadxUnit =
        read_float(
            ini,
            "Coordinates",
            "ERUnitsPerSADXUnit",
            1.0f);

    out.erUnitsPerSadxUnit =
        std::clamp(
            out.erUnitsPerSadxUnit,
            0.001f,
            1000.0f);

    out.swapYZ =
        read_bool(
            ini,
            "Coordinates",
            "SwapYZ",
            false);

    out.invertX =
        read_bool(
            ini,
            "Coordinates",
            "InvertX",
            false);

    out.invertZ =
        read_bool(
            ini,
            "Coordinates",
            "InvertZ",
            false);

    return out;
}

} // namespace ringpass::sadx
