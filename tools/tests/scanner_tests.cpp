#include "module_scanner.hpp"
#include <array>
#include <cstdio>
#include <cstring>

int main()
{
    // Synthetic mapped PE: exercise the real scanner without loading a game.
    alignas(16) std::array<std::uint8_t, 1024> image{};
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image.data());
    dos->e_magic = IMAGE_DOS_SIGNATURE;
    dos->e_lfanew = 128;
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(image.data() + 128);
    nt->Signature = IMAGE_NT_SIGNATURE;
    nt->FileHeader.NumberOfSections = 1;
    nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
    auto* section = IMAGE_FIRST_SECTION(nt);
    section->VirtualAddress = 512;
    section->Misc.VirtualSize = 64;
    section->Characteristics = IMAGE_SCN_MEM_EXECUTE;
    const std::uint8_t bytes[] = {0x48, 0x8B, 0x05, 0xAB};
    std::memcpy(image.data() + 520, bytes, sizeof(bytes));
    ringpass::er::ModuleScanner scanner;
    if (!scanner.initialize(reinterpret_cast<HMODULE>(image.data()))) return 1;
    if (scanner.find("48 8B 05 ??") != reinterpret_cast<std::uintptr_t>(image.data() + 520)) return 2;
    if (scanner.find("FF EE") || scanner.find("GG") || scanner.find("")) return 3;
    std::memcpy(image.data() + 540, bytes, sizeof(bytes));
    if (scanner.find("48 8B 05 ??")) return 4;
    // The second hit can be in a different executable section.
    nt->FileHeader.NumberOfSections = 2;
    section[0].Misc.VirtualSize = 16;
    section[1].VirtualAddress = 536;
    section[1].Misc.VirtualSize = 16;
    section[1].Characteristics = IMAGE_SCN_MEM_EXECUTE;
    if (!scanner.initialize(reinterpret_cast<HMODULE>(image.data()))) return 5;
    if (scanner.find("48 8B 05 ??")) return 6;
    section[1].Characteristics = 0;
    if (!scanner.initialize(reinterpret_cast<HMODULE>(image.data()))) return 7;
    if (!scanner.find("48 8B 05 ??")) return 8;
    std::puts("scanner: unique, absent, invalid, duplicate and section filtering passed");
}
