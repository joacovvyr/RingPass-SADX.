#include <windows.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

#include <ringpass/ipc.hpp>

int main()
{
    ringpass::SharedMemory ipc;
    if (!ipc.open_or_create())
    {
        std::cerr << "FakeSADX: failed to open IPC.\n";
        return 1;
    }

    std::cout << "FakeSADX running. Simulating Sonic. Ctrl+C to stop.\n";

    std::uint64_t frame = 0;

    while (true)
    {
        const float t = static_cast<float>(frame) / 60.0f;

        auto& sadx = ipc.get()->sadx;
        ringpass::begin_write(sadx);

        sadx.protocolVersion = ringpass::kProtocolVersion;
        sadx.frame = ++frame;
        sadx.heartbeatMs = GetTickCount64();
        sadx.player.character = ringpass::CharacterId::Sonic;
        sadx.player.position = {
            std::cos(t) * 12.0f,
            2.0f + std::sin(t * 2.0f) * 0.5f,
            std::sin(t) * 12.0f
        };
        sadx.player.velocity = {
            -std::sin(t) * 12.0f,
            std::cos(t * 2.0f),
            std::cos(t) * 12.0f
        };
        sadx.player.rotation = { 0.0f, t, 0.0f };
        sadx.player.action = 6;
        sadx.player.animation = 14;
        sadx.player.rings = 23;
        sadx.player.grounded = 1;

        ringpass::end_write(sadx);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
