#include <windows.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include <ringpass/ipc.hpp>

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

const char* HostName(ringpass::HostState state)
{
    switch (state)
    {
    case ringpass::HostState::Booting: return "BOOTING";
    case ringpass::HostState::Menu: return "MENU";
    case ringpass::HostState::InWorld: return "IN WORLD";
    default: return "OFFLINE";
    }
}

} // namespace

int main()
{
    std::cout << "RingPass Monitor 0.2\n";
    std::cout << "Waiting for IPC...\n";

    ringpass::SharedMemory ipc;
    while (!ipc.open_existing(FILE_MAP_READ))
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    while (true)
    {
        ringpass::SadxToErChannel sadx{};
        ringpass::ErToSadxChannel er{};

        const auto* state = ipc.get();
        const bool sadxOk = ringpass::read_stable(state->sadx, sadx);
        const bool erOk = ringpass::read_stable(state->er, er);

        std::cout << "\x1b[2J\x1b[H";
        std::cout << "RingPass Monitor 0.2\n\n";

        if (sadxOk)
        {
            const auto& p = sadx.player;
            std::cout << "[SADX -> ER] CONNECTED\n";
            std::cout << "Frame:     " << sadx.frame << "\n";
            std::cout << "Character: " << CharacterName(p.character) << "\n";
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "Position:  " << p.position.x << " / "
                      << p.position.y << " / " << p.position.z << "\n";
            std::cout << "Velocity:  " << p.velocity.x << " / "
                      << p.velocity.y << " / " << p.velocity.z << "\n";
            std::cout << "Action:    " << p.action << "\n";
            std::cout << "Animation: " << p.animation << "\n";
            std::cout << "Grounded:  " << (p.grounded ? "YES" : "NO") << "\n";
            std::cout << "Rings:     " << p.rings << "\n";
        }
        else
        {
            std::cout << "[SADX -> ER] NO STABLE FRAME\n";
        }

        std::cout << "\n";

        if (erOk)
        {
            std::cout << "[ER -> SADX] " << HostName(er.hostState) << "\n";
            std::cout << "Frame:     " << er.frame << "\n";
            std::cout << "Ground:    " << (er.groundProbe.hit ? "HIT" : "MISS") << "\n";
            std::cout << "Targets:   " << er.targetCount << "\n";
        }
        else
        {
            std::cout << "[ER -> SADX] NO STABLE FRAME\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
