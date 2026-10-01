#pragma once

#include <filesystem>

namespace ringpass::sadx {

struct Settings {
    bool injectTargets{false};

    // Elden Ring units represented by one SADX unit.
    // Start at 1.0 for diagnostics and calibrate from real movement tests.
    float erUnitsPerSadxUnit{1.0f};

    bool swapYZ{false};
    bool invertX{false};
    bool invertZ{false};
};

Settings load_settings(
    const std::filesystem::path& modPath);

} // namespace ringpass::sadx
