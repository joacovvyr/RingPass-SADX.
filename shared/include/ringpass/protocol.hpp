#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

namespace ringpass {

constexpr std::uint32_t kProtocolVersion = 1;
constexpr std::size_t kMaxTargets = 128;

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

enum class CharacterId : std::uint32_t {
    Sonic = 0,
    Tails,
    Knuckles,
    Amy,
    Gamma,
    Big,
    Unknown = 0xFFFFFFFFu
};

struct CharacterState {
    std::uint32_t protocolVersion{kProtocolVersion};
    CharacterId character{CharacterId::Unknown};

    Vec3 position{};
    Vec3 velocity{};
    Vec3 rotation{};

    std::int32_t action{-1};
    std::int32_t animation{-1};
    std::int32_t rings{0};

    std::uint8_t grounded{0};
    std::uint8_t reserved[3]{};
};

struct TargetProxy {
    std::uint64_t id{};
    Vec3 position{};
    Vec3 aimPoint{};
    float radius{1.0f};
    float hp{};
    std::uint8_t targetable{1};
    std::uint8_t alive{1};
    std::uint8_t reserved[2]{};
};

struct WorldProbe {
    Vec3 origin{};
    Vec3 hitPosition{};
    Vec3 hitNormal{};
    float distance{};
    std::uint8_t hit{0};
    std::uint8_t reserved[3]{};
};

struct SharedFrame {
    std::uint32_t protocolVersion{kProtocolVersion};
    std::uint64_t sadxFrame{};
    std::uint64_t erFrame{};

    CharacterState player{};

    std::uint32_t targetCount{};
    std::array<TargetProxy, kMaxTargets> targets{};

    WorldProbe groundProbe{};
};

static_assert(std::is_trivially_copyable_v<SharedFrame>);

} // namespace ringpass
