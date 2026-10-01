#include "stable_frame_service.hpp"

#include <windows.h>

namespace ringpass::er {

namespace {

constexpr std::uintptr_t kWorldChrManRva = 0x3D69FF8;
constexpr std::uintptr_t kVtWorldChrMan = 0x2A4E980;
constexpr std::uintptr_t kVtPlayerIns = 0x2A7FBB0;
constexpr std::uintptr_t kVtPhysModule = 0x2A3C890;

constexpr std::size_t kMainPlayer = 0x1E508;
constexpr std::size_t kChrModules = 0x190;
constexpr std::size_t kModPhysics = 0x68;
constexpr std::size_t kPhysPos = 0x70;
constexpr std::size_t kPlayerBlockPos = 0x6C0;
constexpr std::size_t kPlayerBlockId = 0x6D0;

} // namespace

bool StableFrameService::safe_read(
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

bool StableFrameService::validate_vtable(
    const void* object,
    std::uintptr_t expected) const
{
    if (!object || !base_)
        return false;

    std::uintptr_t vtable{};

    return read_value(object, vtable) &&
           vtable == base_ + expected;
}

bool StableFrameService::initialize(
    HostEnvironment& host)
{
    active_ = false;
    base_ = host.game_base();

    if (!host.is_file_version(2, 7, 1, 0))
    {
        host.log().write(
            "stable coordinate frame disabled: exact map/block layout "
            "currently validated only for Elden Ring 2.7.1.0");
        return false;
    }

    if (!base_)
        return false;

    active_ = true;

    host.log().write(
        "stable coordinate frame enabled for ER 2.7.1.0");

    return true;
}

bool StableFrameService::sample(
    StableFrameSample& out) const
{
    out = {};

    if (!active_)
        return false;

    std::uint8_t* world{};

    if (!read_value(
            reinterpret_cast<const void*>(
                base_ + kWorldChrManRva),
            world) ||
        !validate_vtable(
            world,
            kVtWorldChrMan))
        return false;

    std::uint8_t* player{};

    if (!read_value(
            world + kMainPlayer,
            player) ||
        !validate_vtable(
            player,
            kVtPlayerIns))
        return false;

    std::uint32_t block{};

    if (!read_value(
            player + kPlayerBlockId,
            block) ||
        block == 0 ||
        block == 0xFFFFFFFFu)
        return false;

    float blockPosition[3]{};

    if (!safe_read(
            player + kPlayerBlockPos,
            blockPosition,
            sizeof(blockPosition)))
        return false;

    std::uint8_t* modules{};

    if (!read_value(
            player + kChrModules,
            modules) ||
        !modules)
        return false;

    std::uint8_t* physics{};

    if (!read_value(
            modules + kModPhysics,
            physics) ||
        !validate_vtable(
            physics,
            kVtPhysModule))
        return false;

    float havokPosition[3]{};

    if (!safe_read(
            physics + kPhysPos,
            havokPosition,
            sizeof(havokPosition)))
        return false;

    ringpass::Vec3 stable
    {
        blockPosition[0],
        blockPosition[1],
        blockPosition[2]
    };

    const std::uint32_t area =
        block >> 24;

    if (area == 60 || area == 61)
    {
        stable.x +=
            256.0f *
            static_cast<float>(
                (block >> 16) & 0xFF);

        stable.z +=
            256.0f *
            static_cast<float>(
                (block >> 8) & 0xFF);

        out.zone = area << 24;
    }
    else
    {
        out.zone = block;
    }

    out.playerPosition = stable;

    out.havokOffset =
    {
        havokPosition[0] - stable.x,
        havokPosition[1] - stable.y,
        havokPosition[2] - stable.z
    };

    out.valid = true;
    return true;
}

} // namespace ringpass::er
