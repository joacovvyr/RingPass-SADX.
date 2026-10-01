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
        std::cerr << "FakeER: failed to open IPC.\n";
        return 1;
    }

    std::cout << "FakeER running. Ctrl+C to stop.\n";

    std::uint64_t frame = 0;

    while (true)
    {
        ringpass::SadxToErChannel sadx{};
        ringpass::read_stable(ipc.get()->sadx, sadx);

        auto& er = ipc.get()->er;
        ringpass::begin_write(er);

        er.protocolVersion = ringpass::kProtocolVersion;
        er.frame = ++frame;
        er.heartbeatMs = GetTickCount64();
        er.hostState = ringpass::HostState::InWorld;
        er.hostPlayerPosition = sadx.player.position;

        er.groundProbe.origin = sadx.player.position;
        er.groundProbe.hit = 1;
        er.groundProbe.hitPosition = {
            sadx.player.position.x,
            0.0f,
            sadx.player.position.z
        };
        er.groundProbe.hitNormal = { 0.0f, 1.0f, 0.0f };
        er.groundProbe.distance =
            std::fabs(sadx.player.position.y - er.groundProbe.hitPosition.y);

        er.targetCount = 3;

        for (std::uint32_t i = 0; i < er.targetCount; ++i)
        {
            const float angle = static_cast<float>(frame) * 0.01f +
                                static_cast<float>(i) * 2.0943951f;
            auto& t = er.targets[i];

            t.id = 1000 + i;
            t.position = {
                sadx.player.position.x + std::cos(angle) * (8.0f + i * 3.0f),
                sadx.player.position.y + 1.0f,
                sadx.player.position.z + std::sin(angle) * (8.0f + i * 3.0f)
            };
            t.aimPoint = { t.position.x, t.position.y + 1.0f, t.position.z };
            t.radius = 1.0f;
            t.hp = 100.0f;
            t.targetable = 1;
            t.alive = 1;
        }

        ringpass::end_write(er);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
