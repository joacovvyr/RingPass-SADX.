#pragma once

#include <cstddef>
#include <cstdint>

namespace ringpass::er::view {

#pragma pack(push, 1)

struct Vec3 {
    float x;
    float y;
    float z;
};

struct ChrPhysics {
    std::byte pad0[0x70];
    Vec3 localPos;
};

struct ChrModules {
    void* chrData;
    std::byte pad1[0x20];
    void* chrBehavior;
    std::byte pad2[0x38];
    ChrPhysics* chrPhysics;
};

struct ChrIns {
    std::byte pad0[0x190];
    ChrModules* chrModules;
};

struct Players {
    ChrIns* player0;
    std::byte pad0[0x08];
    ChrIns* player1;
};

struct WorldChrMan {
    std::byte pad0[0x10EF8];
    Players* players;
};

struct Camera {
    std::byte pad0[0x10];
    float matrix[16];
    float fovRadians;
};

struct GameRend {
    std::byte pad0[0x18];
    Camera* csPersCam0;
    Camera* csPersCam1;
    Camera* csPersCam2;
};

struct FieldArea {
    std::byte pad0[0x20];
    GameRend* gameRend;
};

#pragma pack(pop)

static_assert(offsetof(ChrPhysics, localPos) == 0x70);
static_assert(offsetof(ChrModules, chrBehavior) == 0x28);
static_assert(offsetof(ChrModules, chrPhysics) == 0x68);
static_assert(offsetof(ChrIns, chrModules) == 0x190);
static_assert(offsetof(Players, player0) == 0x00);
static_assert(offsetof(Players, player1) == 0x10);
static_assert(offsetof(WorldChrMan, players) == 0x10EF8);
static_assert(offsetof(Camera, matrix) == 0x10);
static_assert(offsetof(Camera, fovRadians) == 0x50);
static_assert(offsetof(GameRend, csPersCam0) == 0x18);
static_assert(offsetof(GameRend, csPersCam1) == 0x20);
static_assert(offsetof(GameRend, csPersCam2) == 0x28);
static_assert(offsetof(FieldArea, gameRend) == 0x20);

} // namespace ringpass::er::view
