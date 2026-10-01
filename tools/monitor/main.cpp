#include <windows.h>

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <thread>

#include <ringpass/health.hpp>
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
    std::cout << "RingPass Monitor 0.5\n";
    std::cout << "Waiting for IPC...\n";

    ringpass::SharedMemory ipc;

    while (!ipc.open_existing(FILE_MAP_READ))
        std::this_thread::sleep_for(
            std::chrono::milliseconds(500));

    while (true)
    {
        ringpass::SadxToErChannel sadx{};
        ringpass::ErToSadxChannel er{};

        const auto* state = ipc.get();

        const bool sadxRead =
            ringpass::read_stable(
                state->sadx,
                sadx);

        const bool erRead =
            ringpass::read_stable(
                state->er,
                er);

        const auto sadxHealth =
            sadxRead
                ? ringpass::channel_health(
                    sadx.protocolVersion,
                    sadx.heartbeatMs)
                : ringpass::ChannelHealth::Offline;

        const auto erHealth =
            erRead
                ? ringpass::channel_health(
                    er.protocolVersion,
                    er.heartbeatMs)
                : ringpass::ChannelHealth::Offline;

        std::cout << "\x1b[2J\x1b[H";
        std::cout << "RingPass Monitor 0.4 / protocol "
                  << ringpass::kProtocolVersion
                  << "\n\n";

        std::cout << "[SADX -> ER] "
                  << ringpass::channel_health_name(
                      sadxHealth)
                  << "\n";

        if (sadxRead)
        {
            const auto& p = sadx.player;

            std::cout << "Frame:     "
                      << sadx.frame << "\n";
            std::cout << "Character: "
                      << CharacterName(p.character)
                      << "\n";

            std::cout
                << std::fixed
                << std::setprecision(3);

            std::cout << "Position:  "
                      << p.position.x << " / "
                      << p.position.y << " / "
                      << p.position.z << "\n";

            std::cout << "Velocity:  "
                      << p.velocity.x << " / "
                      << p.velocity.y << " / "
                      << p.velocity.z << "\n";

            std::cout << "Action:    "
                      << p.action << "\n";
            std::cout << "Animation: "
                      << p.animation << "\n";
            std::cout << "Grounded:  "
                      << (p.grounded ? "YES" : "NO")
                      << "\n";
            std::cout << "Rings:     "
                      << p.rings << "\n";
        }

        std::cout << "\n[ER -> SADX] "
                  << ringpass::channel_health_name(
                      erHealth);

        if (erRead)
            std::cout << " / "
                      << HostName(er.hostState);

        std::cout << "\n";

        if (erRead)
        {
            std::cout << "Frame:     "
                      << er.frame << "\n";

            std::cout << "Zone:      0x"
                      << std::hex
                      << er.hostZone
                      << std::dec
                      << "\n";

            std::cout
                << std::fixed
                << std::setprecision(3);

            std::cout << "Player:    "
                      << er.hostPlayerPosition.x
                      << " / "
                      << er.hostPlayerPosition.y
                      << " / "
                      << er.hostPlayerPosition.z
                      << "\n";

            std::cout << "Camera:    "
                      << er.camera.position.x
                      << " / "
                      << er.camera.position.y
                      << " / "
                      << er.camera.position.z
                      << "\n";

            std::cout << "Camera Q:  "
                      << er.camera.rotation.x
                      << " / "
                      << er.camera.rotation.y
                      << " / "
                      << er.camera.rotation.z
                      << " / "
                      << er.camera.rotation.w
                      << "\n";

            std::cout << "FOV(rad):  "
                      << er.camera.fovRadians
                      << "\n";

            std::cout << "Ground:    "
                      << (er.groundProbe.hit
                          ? "HIT"
                          : "MISS")
                      << "\n";

            std::cout << "Targets:   "
                      << er.targetCount
                      << "\n";
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(100));
    }
}
