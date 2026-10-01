#pragma once

#include <windows.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

#include "module_scanner.hpp"
#include <ringpass/logger.hpp>

namespace ringpass::er {

class HostEnvironment {
public:
    HostEnvironment();

    bool initialize();

    [[nodiscard]] bool ready() const { return ready_; }
    [[nodiscard]] const ModuleScanner& scanner() const { return scanner_; }
    [[nodiscard]] HMODULE game_module() const { return gameModule_; }
    [[nodiscard]] std::uintptr_t game_base() const
    {
        return reinterpret_cast<std::uintptr_t>(gameModule_);
    }

    [[nodiscard]] bool has_file_version() const
    {
        return hasFileVersion_;
    }

    [[nodiscard]] const std::array<std::uint16_t, 4>& file_version() const
    {
        return fileVersion_;
    }

    [[nodiscard]] bool is_file_version(
        std::uint16_t a,
        std::uint16_t b,
        std::uint16_t c,
        std::uint16_t d) const
    {
        return hasFileVersion_ &&
               fileVersion_[0] == a &&
               fileVersion_[1] == b &&
               fileVersion_[2] == c &&
               fileVersion_[3] == d;
    }

    [[nodiscard]] std::string file_version_string() const;

    ringpass::Logger& log() { return logger_; }

private:
    static std::filesystem::path log_path();
    bool read_file_version();

    ringpass::Logger logger_;
    ModuleScanner scanner_;
    HMODULE gameModule_{};
    std::array<std::uint16_t, 4> fileVersion_{};
    bool hasFileVersion_{false};
    bool ready_{false};
};

} // namespace ringpass::er
