#include "world_reader.hpp"

#include <windows.h>

#include <cmath>
#include <cstring>
#include <sstream>

namespace ringpass::er {

namespace {

ringpass::Quat quaternion_from_camera_matrix(const float* m)
{
    // Elden Ring stores the camera basis as columns:
    // c0 = right, c1 = up, c2 = forward, c3 = position.
    const float r00 = m[0];
    const float r01 = m[4];
    const float r02 = m[8];
    const float r10 = m[1];
    const float r11 = m[5];
    const float r12 = m[9];
    const float r20 = m[2];
    const float r21 = m[6];
    const float r22 = m[10];

    ringpass::Quat q{};
    const float trace = r00 + r11 + r22;

    if (trace > 0.0f)
    {
        const float s = std::sqrt(trace + 1.0f) * 2.0f;
        q.w = 0.25f * s;
        q.x = (r21 - r12) / s;
        q.y = (r02 - r20) / s;
        q.z = (r10 - r01) / s;
    }
    else if (r00 > r11 && r00 > r22)
    {
        const float s = std::sqrt(1.0f + r00 - r11 - r22) * 2.0f;
        q.w = (r21 - r12) / s;
        q.x = 0.25f * s;
        q.y = (r01 + r10) / s;
        q.z = (r02 + r20) / s;
    }
    else if (r11 > r22)
    {
        const float s = std::sqrt(1.0f + r11 - r00 - r22) * 2.0f;
        q.w = (r02 - r20) / s;
        q.x = (r01 + r10) / s;
        q.y = 0.25f * s;
        q.z = (r12 + r21) / s;
    }
    else
    {
        const float s = std::sqrt(1.0f + r22 - r00 - r11) * 2.0f;
        q.w = (r10 - r01) / s;
        q.x = (r02 + r20) / s;
        q.y = (r12 + r21) / s;
        q.z = 0.25f * s;
    }

    const float len = std::sqrt(
        q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);

    if (len > 0.00001f)
    {
        q.x /= len;
        q.y /= len;
        q.z /= len;
        q.w /= len;
    }
    else
    {
        q = {};
        q.w = 1.0f;
    }

    return q;
}

} // namespace

bool WorldReader::readable(const void* ptr, std::size_t size)
{
    if (!ptr || size == 0)
        return false;

    MEMORY_BASIC_INFORMATION info{};

    if (!VirtualQuery(ptr, &info, sizeof(info)))
        return false;

    if (info.State != MEM_COMMIT)
        return false;

    if (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))
        return false;

    const auto begin =
        reinterpret_cast<std::uintptr_t>(ptr);
    const auto end = begin + size;
    const auto regionEnd =
        reinterpret_cast<std::uintptr_t>(info.BaseAddress) +
        info.RegionSize;

    return end >= begin && end <= regionEnd;
}

std::uintptr_t WorldReader::resolve_rip(
    std::uintptr_t instruction,
    std::size_t displacementOffset,
    std::size_t instructionLength)
{
    const auto* displacementAddress =
        reinterpret_cast<const std::uint8_t*>(
            instruction + displacementOffset);

    std::int32_t displacement{};
    std::memcpy(
        &displacement,
        displacementAddress,
        sizeof(displacement));

    return instruction +
           instructionLength +
           static_cast<std::intptr_t>(displacement);
}

bool WorldReader::initialize(
    const ModuleScanner& scanner,
    ringpass::Logger& logger)
{
    // Patterns verified in current MIT-licensed Elden Ring tooling
    // (September 2026). We still validate them at runtime instead
    // of assuming fixed addresses.
    constexpr const char* kWorldChrManPattern =
        "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 0F 48 39 88";

    constexpr const char* kFieldAreaPattern =
        "48 8B 3D ?? ?? ?? ?? 49 8B D8 48 8B F2 4C 8B F1 48 85 FF";

    const auto worldMatch = scanner.find(kWorldChrManPattern);
    const auto fieldMatch = scanner.find(kFieldAreaPattern);

    if (!worldMatch)
    {
        logger.write("WorldChrMan signature not found");
        return false;
    }

    if (!fieldMatch)
    {
        logger.write("FieldArea signature not found");
        return false;
    }

    worldChrManSlot_ = resolve_rip(*worldMatch, 3, 7);
    fieldAreaSlot_ = resolve_rip(*fieldMatch, 3, 7);

    std::ostringstream line;
    line << "resolved host globals: WorldChrMan slot=0x"
         << std::hex << worldChrManSlot_
         << " FieldArea slot=0x" << fieldAreaSlot_;

    logger.write(line.str());
    return true;
}

bool WorldReader::sample(HostSample& out) const
{
    out = {};

    auto* world =
        read_global_pointer<view::WorldChrMan>(
            worldChrManSlot_);

    if (world &&
        readable(world, sizeof(view::WorldChrMan)) &&
        readable(world->players, sizeof(view::Players)))
    {
        auto* player = world->players->player0;

        if (player &&
            readable(player, sizeof(view::ChrIns)) &&
            readable(
                player->chrModules,
                sizeof(view::ChrModules)))
        {
            auto* physics =
                player->chrModules->chrPhysics;

            if (physics &&
                readable(
                    physics,
                    sizeof(view::ChrPhysics)))
            {
                const auto& p = physics->localPos;
                out.playerPosition = { p.x, p.y, p.z };
                out.playerValid = true;
            }
        }
    }

    auto* field =
        read_global_pointer<view::FieldArea>(
            fieldAreaSlot_);

    if (field &&
        readable(field, sizeof(view::FieldArea)) &&
        readable(
            field->gameRend,
            sizeof(view::GameRend)))
    {
        auto* rend = field->gameRend;

        view::Camera* camera = rend->csPersCam1;

        if (!camera)
            camera = rend->csPersCam0;

        if (!camera)
            camera = rend->csPersCam2;

        if (camera &&
            readable(camera, sizeof(view::Camera)))
        {
            out.camera.position = {
                camera->matrix[12],
                camera->matrix[13],
                camera->matrix[14]
            };

            out.camera.rotation =
                quaternion_from_camera_matrix(
                    camera->matrix);

            out.camera.fovRadians =
                camera->fovRadians;

            out.cameraValid = true;
        }
    }

    return out.playerValid || out.cameraValid;
}

} // namespace ringpass::er
