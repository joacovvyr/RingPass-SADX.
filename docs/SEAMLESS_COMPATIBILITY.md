# Seamless Co-op compatibility plan

RingPass must not take ownership of Elden Ring networking.

## Loader policy

The host bridge is a normal x64 DLL (`RingPassER.dll`). It should be loadable as an external/native DLL by the user's mod stack rather than becoming a `dinput8.dll` proxy itself.

This keeps RingPass from competing for the common proxy-DLL slot and makes it easier to coexist with other mods.

## Initial supported direction

For the first real Elden Ring tests, target ModEngine2-style external DLL loading.

Conceptual configuration:

```toml
external_dlls = [
    "RingPassER.dll"
]
```

When Seamless Co-op is involved, RingPass should remain another independently loaded DLL rather than wrapping or replacing the Seamless DLL.

## Architecture rule

```text
RingPass Core
  |
  +-- EldenRingAdapter
        |
        +-- Base world adapter
        +-- Seamless compatibility adapter
```

The base adapter owns:

- local player transform
- camera
- collision queries
- entity discovery
- host damage bridge
- local player render suppression

The future Seamless adapter owns only information that changes because of multiplayer:

- local/remote ownership
- remote RingPass character identity
- remote transform/animation presentation
- host/client event routing

## Explicit non-goal

RingPass will not implement Easy Anti-Cheat bypasses or target official online play.

Development and DLL-hook testing is for offline/modded launches.
