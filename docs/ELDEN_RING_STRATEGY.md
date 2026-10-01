# Elden Ring integration strategy

## Loader direction

RingPassER is built as an x64 DLL and kept independent from SADX.

The current direction is to support loading it as an external DLL through the Elden Ring modding stack rather than owning a dinput8 proxy slot.

This is deliberate because the project must remain compatible with Seamless Co-op and other host-side mods.

## Current status

RingPassER.dll currently provides:

- x64 bootstrap
- protocol/version export
- shared-memory connection
- host-bridge liveness publication

It deliberately does **not** publish fake Elden Ring world data.

Real world adapters still need validated game-build signatures for:

- player transform
- camera transform/FOV
- collision/raycast access
- nearby entity enumeration
- enemy HP/damage events
- hiding/replacing the local Tarnished render

## Compatibility rule

Do not put multiplayer ownership, Seamless networking, or dinput8 proxy behavior into RingPass Core.

Expected layering:

```text
RingPass Core
    |
    +-- EldenRingAdapter
           |
           +-- Base host hooks
           +-- Seamless compatibility adapter
```

## Offline only while developing hooks

Host-side DLL development is intended for offline/modded launches. RingPass does not target official Elden Ring online play or Easy Anti-Cheat bypass.
