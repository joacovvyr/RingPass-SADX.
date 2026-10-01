#include <windows.h>

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <thread>

#include <ringpass/protocol.hpp>

namespace {

const char* CharacterName(ringpass::CharacterId id)
{
    using ringpass::CharacterId;

    switch (id)
    {
    case CharacterId::Sonic: return "Sonic";
    case CharacterId::Tails: return "Tails";
    case CharacterId::Knuckles: return "Knuckles";
    case CharacterId::Amy: return "Amy";
    case CharacterId::Gamma: return "E-102 Gamma";
    case CharacterId::Big: return "Big";
    default: return "Unknown";
    }
}

bool ReadStableFrame(
    const ringpass::SharedFrame* shared,
    ringpass::SharedFrame& out)
{
    for (int attempt = 0; attempt < 10; ++attempt)
    {
        const std::uint32_t before = shared->sequence;

        if (before & 1u)
            continue;

        MemoryBarrier();
        out = *shared;
        MemoryBarrier();

        const std::uint32_t after = shared->sequence;

        if (before == after && !(after & 1u))
            return true;
    }

    return false;
}

} // namespace

int main()
{
    std::cout << "RingPass SADX Monitor 0.1\n";
    std::cout << "Waiting for RingPassSADX.dll...\n\n";

    HANDLE mapping = nullptr;

    while (!mapping)
    {
        mapping = OpenFileMappingA(
            FILE_MAP_READ,
            FALSE,
            ringpass::kSADXSharedMemoryName);

        if (!mapping)
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    auto* shared = static_cast<const ringpass::SharedFrame*>(
        MapViewOfFile(
            mapping,
            FILE_MAP_READ,
            0,
            0,
            sizeof(ringpass::SharedFrame)));

    if (!shared)
    {
        std::cerr << "Could not map RingPass shared memory.\n";
        CloseHandle(mapping);
        return 1;
    }

    while (true)
    {
        ringpass::SharedFrame frame{};

        if (ReadStableFrame(shared, frame))
        {
            const auto& p = frame.player;

            std::cout << "\x1b[2J\x1b[H";
            std::cout << "RingPass SADX Bridge 0.1\n\n";
            std::cout << "CONNECTED\n";
            std::cout << "Frame:     " << frame.sadxFrame << "\n";
            std::cout << "Character: " << CharacterName(p.character) << "\n";

            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Position:  "
                      << p.position.x << " / "
                      << p.position.y << " / "
                      << p.position.z << "\n";
            std::cout << "Velocity:  "
                      << p.velocity.x << " / "
                      << p.velocity.y << " / "
                      << p.velocity.z << "\n";
            std::cout << "Rotation:  "
                      << p.rotation.x << " / "
                      << p.rotation.y << " / "
                      << p.rotation.z << "\n";

            std::cout << "Action:    " << p.action << "\n";
            std::cout << "Animation: " << p.animation << "\n";
            std::cout << "Grounded:  "
                      << (p.grounded ? "YES" : "NO") << "\n";
            std::cout << "Rings:     " << p.rings << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
