#include "enemy_reader.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace ringpass::er {

namespace {

constexpr std::uintptr_t kWorldChrManRva = 0x3D69FF8;

constexpr std::uintptr_t kVtWorldChrMan = 0x2A4E980;
constexpr std::uintptr_t kVtPlayerIns = 0x2A7FBB0;
constexpr std::uintptr_t kVtEnemyIns = 0x2A47090;
constexpr std::uintptr_t kVtDataModule = 0x2A380B8;
constexpr std::uintptr_t kVtPhysModule = 0x2A3C890;

constexpr std::size_t kMainPlayer = 0x1E508;
constexpr std::size_t kChrsByDistance = 0x1F1D0;
constexpr std::size_t kChrByDistanceEntry = 0x10;

constexpr std::size_t kChrHandle = 0x08;
constexpr std::size_t kChrModelId = 0x64;
constexpr std::size_t kChrTeamType = 0x6C;
constexpr std::size_t kChrModules = 0x190;
constexpr std::size_t kChrRenderFlags = 0x1C5;
constexpr std::size_t kChrDistSq = 0x3FC;

constexpr std::size_t kModData = 0x00;
constexpr std::size_t kModPhysics = 0x68;

constexpr std::size_t kDataHp = 0x138;
constexpr std::size_t kDataMaxHp = 0x13C;

constexpr std::size_t kPhysPos = 0x70;
constexpr std::size_t kPhysHitHeight = 0x2E0;
constexpr std::size_t kPhysHitRadius = 0x2E4;

constexpr float kTargetRadiusMeters = 80.0f;

struct Candidate {
    std::uint8_t* instance{};
    float distanceSq{};
};

} // namespace

bool EnemyReader::safe_read(
    const void* address,
    void* out,
    std::size_t size)
{
    if (!address || !out || !size)
        return false;

    SIZE_T read = 0;

    return ReadProcessMemory(
               GetCurrentProcess(),
               address,
               out,
               size,
               &read) != FALSE &&
           read == size;
}

bool EnemyReader::validate_vtable(
    const void* object,
    std::uintptr_t expected) const
{
    if (!object || !base_)
        return false;

    std::uintptr_t vtable{};

    return read_value(object, vtable) &&
           vtable == base_ + expected;
}

bool EnemyReader::initialize(HostEnvironment& host)
{
    active_ = false;
    base_ = host.game_base();

    if (!host.is_file_version(2, 7, 1, 0))
    {
        host.log().write(
            "enemy reader disabled: exact layouts currently validated "
            "only for Elden Ring file version 2.7.1.0 / App Ver. 1.17.1");
        return false;
    }

    if (!base_)
        return false;

    host.log().write(
        "read-only enemy reader enabled for ER 2.7.1.0");

    active_ = true;
    return true;
}

std::uint32_t EnemyReader::sample(
    ringpass::Vec3,
    ringpass::TargetProxy* out,
    std::uint32_t capacity) const
{
    if (!active_ || !out || capacity == 0)
        return 0;

    std::uint8_t* world{};

    if (!read_value(
            reinterpret_cast<const void*>(
                base_ + kWorldChrManRva),
            world))
        return 0;

    if (!validate_vtable(world, kVtWorldChrMan))
        return 0;

    std::uint8_t* player{};

    if (!read_value(
            world + kMainPlayer,
            player))
        return 0;

    if (!validate_vtable(player, kVtPlayerIns))
        return 0;

    std::uint8_t* begin{};
    std::uint8_t* end{};

    if (!read_value(
            world + kChrsByDistance + 8,
            begin) ||
        !read_value(
            world + kChrsByDistance + 0x10,
            end))
        return 0;

    if (!begin || !end || end <= begin)
        return 0;

    std::size_t count =
        static_cast<std::size_t>(
            end - begin) /
        kChrByDistanceEntry;

    count = std::min<std::size_t>(
        count,
        1024);

    std::uint32_t written = 0;

    for (std::size_t i = 0;
         i < count && written < capacity;
         ++i)
    {
        std::uint8_t* instance{};

        if (!read_value(
                begin + i * kChrByDistanceEntry,
                instance))
            continue;

        if (!instance || instance == player)
            continue;

        if (!validate_vtable(
                instance,
                kVtEnemyIns))
            continue;

        float distanceSq{};

        if (!read_value(
                instance + kChrDistSq,
                distanceSq))
            continue;

        if (!std::isfinite(distanceSq) ||
            distanceSq < 0.0f ||
            distanceSq >
                kTargetRadiusMeters *
                kTargetRadiusMeters)
            continue;

        std::uint8_t team{};

        if (!read_value(
                instance + kChrTeamType,
                team))
            continue;

        if (team != 6 && team != 7)
            continue;

        int model{};

        if (!read_value(
                instance + kChrModelId,
                model))
            continue;

        if (model == 100 || model == 1000)
            continue;

        std::uint8_t* modules{};

        if (!read_value(
                instance + kChrModules,
                modules) ||
            !modules)
            continue;

        std::uint8_t* data{};
        std::uint8_t* physics{};

        if (!read_value(
                modules + kModData,
                data) ||
            !read_value(
                modules + kModPhysics,
                physics))
            continue;

        if (!validate_vtable(
                data,
                kVtDataModule) ||
            !validate_vtable(
                physics,
                kVtPhysModule))
            continue;

        int hp{};
        int maxHp{};

        if (!read_value(
                data + kDataHp,
                hp) ||
            !read_value(
                data + kDataMaxHp,
                maxHp))
            continue;

        if (maxHp <= 0)
            continue;

        float position[3]{};

        if (!safe_read(
                physics + kPhysPos,
                position,
                sizeof(position)))
            continue;

        float height{};
        float radius{};

        read_value(
            physics + kPhysHitHeight,
            height);

        read_value(
            physics + kPhysHitRadius,
            radius);

        if (!std::isfinite(height) ||
            height < 0.05f ||
            height > 40.0f)
            height = 1.8f;

        if (!std::isfinite(radius) ||
            radius < 0.05f ||
            radius > 15.0f)
            radius = 0.4f;

        std::uint64_t handle{};

        if (!read_value(
                instance + kChrHandle,
                handle) ||
            !handle)
            continue;

        std::uint8_t renderFlags{};
        read_value(
            instance + kChrRenderFlags,
            renderFlags);

        auto& target = out[written++];
        target = {};

        target.id = handle;
        target.position =
        {
            position[0],
            position[1],
            position[2]
        };

        target.aimPoint =
        {
            position[0],
            position[1] + height * 0.6f,
            position[2]
        };

        target.radius = radius;
        target.hp =
            static_cast<float>(
                std::max(hp, 0));

        target.alive =
            hp > 0 &&
            !(renderFlags & 0x80)
                ? 1
                : 0;

        target.targetable =
            target.alive ? 1 : 0;
    }

    return written;
}

} // namespace ringpass::er
