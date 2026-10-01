#include "module_scanner.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>

namespace ringpass::er {

namespace {

struct Pattern {
    std::vector<std::uint8_t> bytes;
    std::vector<std::uint8_t> mask;
};

std::optional<Pattern> parse_pattern(const char* text)
{
    if (!text)
        return std::nullopt;

    Pattern out;
    const char* p = text;

    while (*p)
    {
        while (*p && std::isspace(static_cast<unsigned char>(*p)))
            ++p;

        if (!*p)
            break;

        if (p[0] == '?' && p[1] == '?')
        {
            out.bytes.push_back(0);
            out.mask.push_back(0);
            p += 2;
            continue;
        }

        if (!std::isxdigit(static_cast<unsigned char>(p[0])) ||
            !std::isxdigit(static_cast<unsigned char>(p[1])))
            return std::nullopt;

        char token[3]{ p[0], p[1], 0 };
        out.bytes.push_back(
            static_cast<std::uint8_t>(std::strtoul(token, nullptr, 16)));
        out.mask.push_back(0xFF);
        p += 2;
    }

    if (out.bytes.empty())
        return std::nullopt;

    return out;
}

} // namespace

bool ModuleScanner::initialize(HMODULE module)
{
    regions_.clear();
    fingerprint_ = {};

    if (!module)
        return false;

    auto* base = reinterpret_cast<std::uint8_t*>(module);
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);

    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(
        base + dos->e_lfanew);

    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return false;

    fingerprint_.peTimestamp = nt->FileHeader.TimeDateStamp;
    fingerprint_.imageSize = nt->OptionalHeader.SizeOfImage;
    fingerprint_.checksum = nt->OptionalHeader.CheckSum;

    auto* section = IMAGE_FIRST_SECTION(nt);

    for (std::uint16_t i = 0; i < nt->FileHeader.NumberOfSections; ++i)
    {
        if (!(section[i].Characteristics & IMAGE_SCN_MEM_EXECUTE))
            continue;

        const std::size_t size =
            section[i].Misc.VirtualSize != 0
                ? section[i].Misc.VirtualSize
                : section[i].SizeOfRawData;

        if (!size)
            continue;

        regions_.push_back({
            base + section[i].VirtualAddress,
            size
        });
    }

    return !regions_.empty();
}

std::optional<std::uintptr_t> ModuleScanner::find(
    const char* patternText) const
{
    const auto pattern = parse_pattern(patternText);
    if (!pattern)
        return std::nullopt;

    const std::size_t count = pattern->bytes.size();

    for (const auto& region : regions_)
    {
        if (!region.address || region.size < count)
            continue;

        const std::size_t limit = region.size - count;

        for (std::size_t i = 0; i <= limit; ++i)
        {
            bool match = true;

            for (std::size_t j = 0; j < count; ++j)
            {
                if (pattern->mask[j] &&
                    region.address[i + j] != pattern->bytes[j])
                {
                    match = false;
                    break;
                }
            }

            if (match)
                return reinterpret_cast<std::uintptr_t>(
                    region.address + i);
        }
    }

    return std::nullopt;
}

} // namespace ringpass::er
