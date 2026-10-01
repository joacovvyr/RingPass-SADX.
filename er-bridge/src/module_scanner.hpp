#pragma once

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ringpass::er {

struct ExecutableRegion {
    std::uint8_t* address{};
    std::size_t size{};
};

struct ModuleFingerprint {
    std::uint32_t peTimestamp{};
    std::uint32_t imageSize{};
    std::uint32_t checksum{};
};

class ModuleScanner {
public:
    bool initialize(HMODULE module);

    [[nodiscard]] const ModuleFingerprint& fingerprint() const
    {
        return fingerprint_;
    }

    [[nodiscard]] const std::vector<ExecutableRegion>& regions() const
    {
        return regions_;
    }

    std::optional<std::uintptr_t> find(const char* pattern) const;

private:
    ModuleFingerprint fingerprint_{};
    std::vector<ExecutableRegion> regions_;
};

} // namespace ringpass::er
