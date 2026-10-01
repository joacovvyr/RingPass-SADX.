# Building the SADX bridge

## Requirements

- Visual Studio 2022 with Desktop development with C++
- CMake 3.24+
- A checkout of the current X-Hax/sadx-mod-loader repository
- SADX configured through SA Mod Manager / SADX Mod Loader

SADX mods are 32-bit. Generate the project with the **Win32** architecture.

## Clone the loader SDK

Example:

```powershell
git clone https://github.com/X-Hax/sadx-mod-loader.git C:\\dev\\sadx-mod-loader
```

## Configure

From the RingPass-SADX repository:

```powershell
cmake -S . -B build -A Win32 -DSADX_MOD_LOADER_ROOT=C:\\dev\\sadx-mod-loader
```

## Build

```powershell
cmake --build build --config Release
```

The build creates:

```text
build/
└── package/
    ├── RingPass-SADX/
    │   ├── mod.ini
    │   └── RingPassSADX.dll
    └── tools/
        └── RingPassMonitor.exe
```

## Install into SADX

Copy the generated `RingPass-SADX` folder into the SADX `mods` directory, enable it in SA Mod Manager, then launch the game.

Run:

```text
RingPassMonitor.exe
```

The monitor waits for the mod DLL and then shows the live player state.

Expected output:

```text
CONNECTED
Character: Sonic
Position:  123.00 / 10.00 / -35.00
Velocity:  2.40 / 0.00 / 5.10
Action:    ...
Animation: ...
Grounded:  YES
Rings:     ...
```

## What this proves

This milestone validates that RingPass can obtain the original SADX runtime state from another process without recreating Sonic's movement logic.

The next milestone is the Elden Ring host adapter.
