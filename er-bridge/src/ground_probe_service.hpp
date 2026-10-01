#pragma once

#include <windows.h>

#include <atomic>
#include <cstdint>

#include "host_environment.hpp"

#include <ringpass/protocol.hpp>

namespace ringpass::er {

class GroundProbeService {
public:
    bool initialize(HostEnvironment& host);
    void shutdown();

    void request(ringpass::Vec3 origin);
    bool read_latest(ringpass::WorldProbe& out) const;

    [[nodiscard]] bool active() const { return active_; }

private:
    struct TaskObject {
        void** vtable{};
        std::uint64_t unk8{};
        std::uint64_t reserved[6]{};
    };

    struct TaskPage {
        std::uint64_t magic{};
        void* target{};
        void* vtable[3]{};
        TaskObject task{};
        std::uint8_t stubRet0[4]{};
        std::uint8_t stubRet[4]{};
        std::uint8_t stubExec[16]{};
    };

    struct Exchange {
        volatile LONG requestSeq{0};
        volatile LONG responseSeq{0};
        float origin[3]{};
        ringpass::WorldProbe result{};
    };

    using RegisterTaskFn =
        bool(__fastcall*)(void*, std::uint32_t, void*);

    using CastRayFn =
        bool(__fastcall*)(
            void*,
            std::uint32_t,
            const float*,
            const float*,
            float*,
            void*);

    static void __fastcall task_execute(
        void* self,
        const void* data);

    static GroundProbeService* instance_;

    bool install_task();
    bool service_one();
    void* havok_world() const;

    HostEnvironment* host_{};
    TaskPage* taskPage_{};
    RegisterTaskFn registerTask_{};
    CastRayFn castRay_{};

    std::uintptr_t base_{};
    std::uintptr_t havokManSlot_{};
    Exchange exchange_{};

    bool active_{false};
};

} // namespace ringpass::er
