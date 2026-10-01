#include <windows.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include <ringpass/health.hpp>
#include <ringpass/ipc.hpp>

namespace {

std::filesystem::path exe_dir()
{
    wchar_t buffer[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}

} // namespace

int main(int argc, char** argv)
{
    int seconds = 15;
    if (argc >= 2)
    {
        const int parsed = std::atoi(argv[1]);
        if (parsed > 0 && parsed <= 300)
            seconds = parsed;
    }

    const auto dir = exe_dir();
    const auto csvPath = dir / "RingPass-Capture.csv";

    std::ofstream csv(csvPath, std::ios::trunc);
    if (!csv)
    {
        std::cerr << "Could not create capture file.\n";
        return 1;
    }

    csv << "time_ms,"
           "sadx_health,sadx_frame,character,"
           "sadx_x,sadx_y,sadx_z,"
           "vel_x,vel_y,vel_z,action,animation,rings,grounded,"
           "er_health,er_frame,host_state,host_zone,"
           "er_x,er_y,er_z,"
           "cam_x,cam_y,cam_z,fov,"
           "ground_hit,ground_distance,target_count,"
           "target0_id,target0_x,target0_y,target0_z,target0_targetable,target0_alive,"
           "target1_id,target1_x,target1_y,target1_z,target1_targetable,target1_alive,"
           "target2_id,target2_x,target2_y,target2_z,target2_targetable,target2_alive\n";

    ringpass::SharedMemory ipc;

    const auto start = std::chrono::steady_clock::now();
    const auto end = start + std::chrono::seconds(seconds);

    std::cout << "Capturing RingPass telemetry for "
              << seconds << " seconds...\n";

    while (std::chrono::steady_clock::now() < end)
    {
        ringpass::SadxToErChannel sadx{};
        ringpass::ErToSadxChannel er{};

        auto sadxHealth = ringpass::ChannelHealth::Offline;
        auto erHealth = ringpass::ChannelHealth::Offline;

        if (ipc.open_existing(FILE_MAP_READ))
        {
            if (ringpass::read_stable(ipc.get()->sadx, sadx))
                sadxHealth = ringpass::channel_health(
                    sadx.protocolVersion,
                    sadx.heartbeatMs);

            if (ringpass::read_stable(ipc.get()->er, er))
                erHealth = ringpass::channel_health(
                    er.protocolVersion,
                    er.heartbeatMs);
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - start).count();

        csv << elapsed << ","
            << ringpass::channel_health_name(sadxHealth) << ","
            << sadx.frame << ","
            << static_cast<std::uint32_t>(sadx.player.character) << ","
            << sadx.player.position.x << ","
            << sadx.player.position.y << ","
            << sadx.player.position.z << ","
            << sadx.player.velocity.x << ","
            << sadx.player.velocity.y << ","
            << sadx.player.velocity.z << ","
            << sadx.player.action << ","
            << sadx.player.animation << ","
            << sadx.player.rings << ","
            << static_cast<int>(sadx.player.grounded) << ","
            << ringpass::channel_health_name(erHealth) << ","
            << er.frame << ","
            << static_cast<std::uint32_t>(er.hostState) << ","
            << er.hostZone << ","
            << er.hostPlayerPosition.x << ","
            << er.hostPlayerPosition.y << ","
            << er.hostPlayerPosition.z << ","
            << er.camera.position.x << ","
            << er.camera.position.y << ","
            << er.camera.position.z << ","
            << er.camera.fovRadians << ","
            << static_cast<int>(er.groundProbe.hit) << ","
            << er.groundProbe.distance << ","
            << er.targetCount;

        for (std::uint32_t i = 0; i < 3; ++i)
        {
            const auto& t = er.targets[i];
            csv << ","
                << t.id << ","
                << t.position.x << ","
                << t.position.y << ","
                << t.position.z << ","
                << static_cast<int>(t.targetable) << ","
                << static_cast<int>(t.alive);
        }

        csv << "\n";

        std::this_thread::sleep_for(
            std::chrono::milliseconds(100));
    }

    csv.flush();

    std::cout << "Capture complete: "
              << csvPath.string() << "\n";
    return 0;
}
