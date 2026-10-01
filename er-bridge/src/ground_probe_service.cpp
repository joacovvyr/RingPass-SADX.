#include "ground_probe_service.hpp"

#include <cmath>
#include <cstring>
#include <sstream>

namespace ringpass::er {

namespace {

constexpr std::uintptr_t kCshavokManRva = 0x3D7A0D0;
constexpr std::uintptr_t kCsTaskImpRva = 0x458FE88;
constexpr std::uintptr_t kTaskImpVtableRva = 0x2C04B00;
constexpr std::size_t kHavokPhysWorldOffset = 0x98;

constexpr std::uint32_t kWorldChrManPostPhysicsGroup = 117;
constexpr std::uint32_t kTerrainRayFilter = 0x5D;

constexpr const char* kRegisterTaskPattern =
    "48 89 5C 24 08 57 48 83 EC 40 "
    "48 8D 4C 24 20 49 8B D8 8B FA E8";

constexpr const char* kCastRayPattern =
    "40 55 56 57 41 54 41 55 41 56 41 57 "
    "48 8D AC 24 70 FF FF FF 48 81 EC 90 01 00 00";

bool readable(const void* ptr, std::size_t size)
{
    if (!ptr || !size)
        return false;

    MEMORY_BASIC_INFORMATION info{};

    if (!VirtualQuery(ptr, &info, sizeof(info)))
        return false;

    if (info.State != MEM_COMMIT)
        return false;

    if (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))
        return false;

    const auto begin =
        reinterpret_cast<std::uintptr_t>(ptr);
    const auto end = begin + size;
    const auto regionEnd =
        reinterpret_cast<std::uintptr_t>(info.BaseAddress) +
        info.RegionSize;

    return end >= begin && end <= regionEnd;
}

} // namespace

GroundProbeService* GroundProbeService::instance_ = nullptr;

bool GroundProbeService::initialize(HostEnvironment& host)
{
    shutdown();

    host_ = &host;
    base_ = host.game_base();

    if (!host.is_file_version(2, 7, 1, 0))
    {
        host.log().write(
            "ground probe disabled: only Elden Ring file version "
            "2.7.1.0 / App Ver. 1.17.1 is currently validated; detected " +
            host.file_version_string());
        return false;
    }

    const auto registerTask =
        host.scanner().find(kRegisterTaskPattern);

    const auto castRay =
        host.scanner().find(kCastRayPattern);

    if (!registerTask || !castRay)
    {
        host.log().write(
            "ground probe disabled: required game-thread signatures "
            "were not found");
        return false;
    }

    registerTask_ =
        reinterpret_cast<RegisterTaskFn>(*registerTask);

    castRay_ =
        reinterpret_cast<CastRayFn>(*castRay);

    havokManSlot_ =
        base_ + kCshavokManRva;

    instance_ = this;

    if (!install_task())
    {
        instance_ = nullptr;
        host.log().write(
            "ground probe disabled: failed to register game-thread task");
        return false;
    }

    active_ = true;

    std::ostringstream line;
    line << "ground probe enabled for ER 2.7.1.0; RegisterTask=0x"
         << std::hex << *registerTask
         << " CastRay=0x" << *castRay;

    host.log().write(line.str());
    return true;
}

void GroundProbeService::shutdown()
{
    active_ = false;

    if (taskPage_)
    {
        InterlockedExchangePointer(
            reinterpret_cast<void* volatile*>(&taskPage_->target),
            nullptr);
    }

    if (instance_ == this)
        instance_ = nullptr;

    taskPage_ = nullptr;
    registerTask_ = nullptr;
    castRay_ = nullptr;
    host_ = nullptr;
    base_ = 0;
    havokManSlot_ = 0;
}

bool GroundProbeService::install_task()
{
    if (!registerTask_ || !base_)
        return false;

    auto** taskImpSlot =
        reinterpret_cast<void**>(
            base_ + kCsTaskImpRva);

    if (!readable(taskImpSlot, sizeof(void*)))
        return false;

    void* taskImp = *taskImpSlot;

    if (!readable(taskImp, sizeof(void*)))
        return false;

    const auto vtable =
        *reinterpret_cast<std::uintptr_t*>(taskImp);

    if (vtable != base_ + kTaskImpVtableRva)
    {
        if (host_)
            host_->log().write(
                "CSTaskImp vtable mismatch; refusing to register task");
        return false;
    }

    taskPage_ =
        static_cast<TaskPage*>(
            VirtualAlloc(
                nullptr,
                0x1000,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE));

    if (!taskPage_)
        return false;

    std::memset(taskPage_, 0, sizeof(TaskPage));

    constexpr std::uint8_t kRet0[4] =
        { 0x31, 0xC0, 0xC3, 0xCC };

    constexpr std::uint8_t kRet[4] =
        { 0xC3, 0xCC, 0xCC, 0xCC };

    std::memcpy(taskPage_->stubRet0, kRet0, 4);
    std::memcpy(taskPage_->stubRet, kRet, 4);

    auto* stub = taskPage_->stubExec;

    const std::uint8_t code[16] =
    {
        0x48, 0x8B, 0x05, 0, 0, 0, 0,
        0x48, 0x85, 0xC0,
        0x74, 0x02,
        0xFF, 0xE0,
        0xC3, 0xCC
    };

    std::memcpy(stub, code, sizeof(code));

    const auto targetAddress =
        reinterpret_cast<std::uint8_t*>(
            &taskPage_->target);

    const auto nextInstruction =
        stub + 7;

    const std::intptr_t delta =
        targetAddress - nextInstruction;

    if (delta < INT32_MIN || delta > INT32_MAX)
        return false;

    const auto displacement =
        static_cast<std::int32_t>(delta);

    std::memcpy(
        stub + 3,
        &displacement,
        sizeof(displacement));

    taskPage_->vtable[0] =
        taskPage_->stubRet0;

    taskPage_->vtable[1] =
        taskPage_->stubRet;

    taskPage_->vtable[2] =
        taskPage_->stubExec;

    taskPage_->task.vtable =
        taskPage_->vtable;

    FlushInstructionCache(
        GetCurrentProcess(),
        taskPage_,
        sizeof(TaskPage));

    const bool registered =
        registerTask_(
            taskImp,
            kWorldChrManPostPhysicsGroup,
            &taskPage_->task);

    if (!registered)
        return false;

    InterlockedExchangePointer(
        reinterpret_cast<void* volatile*>(&taskPage_->target),
        reinterpret_cast<void*>(&GroundProbeService::task_execute));

    return true;
}

void GroundProbeService::request(ringpass::Vec3 origin)
{
    if (!active_)
        return;

    const LONG next =
        exchange_.requestSeq + 1;

    exchange_.origin[0] = origin.x;
    exchange_.origin[1] = origin.y;
    exchange_.origin[2] = origin.z;

    MemoryBarrier();

    InterlockedExchange(
        &exchange_.requestSeq,
        next);
}

bool GroundProbeService::read_latest(
    ringpass::WorldProbe& out) const
{
    if (!active_)
        return false;

    const LONG request =
        exchange_.requestSeq;

    const LONG response =
        exchange_.responseSeq;

    if (request == 0 || response != request)
        return false;

    MemoryBarrier();
    out = exchange_.result;
    MemoryBarrier();

    return exchange_.responseSeq == response;
}

void __fastcall GroundProbeService::task_execute(
    void*,
    const void*)
{
    if (instance_)
        instance_->service_one();
}

void* GroundProbeService::havok_world() const
{
    if (!havokManSlot_ ||
        !readable(
            reinterpret_cast<void*>(havokManSlot_),
            sizeof(void*)))
        return nullptr;

    auto* havok =
        *reinterpret_cast<std::uint8_t**>(
            havokManSlot_);

    if (!havok ||
        !readable(
            havok + kHavokPhysWorldOffset,
            sizeof(void*)))
        return nullptr;

    return *reinterpret_cast<void**>(
        havok + kHavokPhysWorldOffset);
}

bool GroundProbeService::service_one()
{
    if (!active_ || !castRay_)
        return false;

    const LONG request =
        exchange_.requestSeq;

    if (request == 0 ||
        exchange_.responseSeq == request)
        return false;

    float source[3]{};

    MemoryBarrier();
    source[0] = exchange_.origin[0];
    source[1] = exchange_.origin[1];
    source[2] = exchange_.origin[2];
    MemoryBarrier();

    void* world = havok_world();

    ringpass::WorldProbe result{};
    result.origin = {
        source[0],
        source[1],
        source[2]
    };

    if (world)
    {
        alignas(16) float origin[4] =
        {
            source[0],
            source[1] + 2.0f,
            source[2],
            1.0f
        };

        alignas(16) float delta[4] =
        {
            0.0f,
            -8.0f,
            0.0f,
            0.0f
        };

        alignas(16) float hit[4]{};

        const bool didHit =
            castRay_(
                world,
                kTerrainRayFilter,
                origin,
                delta,
                hit,
                nullptr);

        if (didHit)
        {
            result.hit = 1;
            result.hitPosition =
            {
                hit[0],
                hit[1],
                hit[2]
            };

            result.hitNormal =
            {
                0.0f,
                1.0f,
                0.0f
            };

            result.distance =
                std::fabs(
                    origin[1] - hit[1]);
        }
    }

    exchange_.result = result;

    MemoryBarrier();

    InterlockedExchange(
        &exchange_.responseSeq,
        request);

    return true;
}

} // namespace ringpass::er
