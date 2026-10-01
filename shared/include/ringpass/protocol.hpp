#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ringpass {

constexpr std::uint32_t kProtocolVersion = 4;
constexpr std::uint32_t kSharedMagic = 0x52505358u; // RPSX
constexpr std::size_t kMaxTargets = 128;
constexpr std::uint64_t kHeartbeatTimeoutMs = 2000;
constexpr const char* kSharedMemoryName =
    "Local\\RingPassSADX_SharedState_v4";

#pragma pack(push, 4)

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

struct Quat {
    float x{};
    float y{};
    float z{};
    float w{1.0f};
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

enum class HostState : std::uint32_t {
    Offline = 0,
    Booting,
    Menu,
    InWorld
};

struct CharacterState {
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
    Vec3 hitNormal{0.0f, 1.0f, 0.0f};
    float distance{};
    std::uint8_t hit{0};
    std::uint8_t reserved[3]{};
};

struct CameraState {
    Vec3 position{};
    Quat rotation{};
    float fovRadians{1.0f};
};

struct SadxToErChannel {
    volatile std::uint32_t sequence{0};
    std::uint32_t protocolVersion{kProtocolVersion};
    std::uint64_t frame{};
    std::uint64_t heartbeatMs{};
    CharacterState player{};
};

struct ErToSadxChannel {
    volatile std::uint32_t sequence{0};
    std::uint32_t protocolVersion{kProtocolVersion};
    std::uint64_t frame{};
    std::uint64_t heartbeatMs{};
    HostState hostState{HostState::Offline};
    Vec3 hostPlayerPosition{};
    CameraState camera{};
    WorldProbe groundProbe{};
    std::uint32_t targetCount{};
    std::array<TargetProxy, kMaxTargets> targets{};
};

struct SharedState {
    std::uint32_t magic{kSharedMagic};
    std::uint32_t protocolVersion{kProtocolVersion};
    SadxToErChannel sadx{};
    ErToSadxChannel er{};
};

#pragma pack(pop)

static_assert(std::is_trivially_copyable_v<Vec3>);
static_assert(std::is_trivially_copyable_v<CharacterState>);
static_assert(std::is_trivially_copyable_v<SharedState>);

static_assert(sizeof(Vec3) == 12);
static_assert(sizeof(Quat) == 16);
static_assert(sizeof(CharacterState) == 56);
static_assert(sizeof(TargetProxy) == 44);
static_assert(sizeof(WorldProbe) == 44);
static_assert(sizeof(CameraState) == 32);
static_assert(sizeof(SadxToErChannel) == 80);
static_assert(sizeof(ErToSadxChannel) == 5752);
static_assert(sizeof(SharedState) == 5840);

static_assert(offsetof(SadxToErChannel, sequence) == 0);
static_assert(offsetof(SadxToErChannel, protocolVersion) == 4);
static_assert(offsetof(SadxToErChannel, frame) == 8);
static_assert(offsetof(SadxToErChannel, heartbeatMs) == 16);

static_assert(offsetof(ErToSadxChannel, sequence) == 0);
static_assert(offsetof(ErToSadxChannel, protocolVersion) == 4);
static_assert(offsetof(ErToSadxChannel, frame) == 8);
static_assert(offsetof(ErToSadxChannel, heartbeatMs) == 16);

} // namespace ringpass
