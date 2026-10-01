# RingPass-SADX

Passthrough bridge experiment: run **Sonic Adventure DX** and **Elden Ring** at the same time, preserving SADX character gameplay while using Elden Ring as the host world.

## Core rule

SADX remains authoritative for:
- character movement
- actions and animations
- character-specific abilities
- targeting logic (e.g. Sonic Homing Attack, Gamma lock-on)

Elden Ring remains authoritative for:
- world geometry
- terrain and collision source
- enemies and bosses
- host-world HP/damage
- streaming/rendering

## Initial characters

- Sonic
- Tails
- Knuckles
- Amy
- E-102 Gamma
- Big

## Architecture

```text
SADX (32-bit)                         Elden Ring (64-bit)
     |                                      |
RingPassSADX.dll                        RingPassER.dll
     |                                      |
     +---------- Shared Memory IPC ----------+
                 |
            RingPass tools
```

IPC v2 uses separate writer channels:

- SADX -> Elden Ring: character state
- Elden Ring -> SADX: host state, camera, collision probes, target proxies

This avoids both games writing the same frame structure.

## Current development status

- [x] repository skeleton
- [x] SADX Mod Loader integration
- [x] active SADX character/state telemetry
- [x] bidirectional IPC v3 with heartbeats / stale detection
- [x] separate x86 SADX / x64 Elden Ring builds
- [x] coordinate mapping layer
- [x] fixed x86/x64 binary IPC layout
- [x] protocol/coordinate/health tests
- [x] synthetic SADX target-proxy builder
- [x] RingPass monitor
- [x] FakeSADX simulator
- [x] FakeER simulator
- [x] launcher/bridge diagnostics
- [x] Elden Ring x64 bridge bootstrap
- [ ] validated Elden Ring player transform hook
- [ ] validated Elden Ring camera hook
- [ ] real Elden Ring ground probe
- [ ] real nearby enemy enumeration
- [ ] live SADX target-list injection (builder complete; hook timing pending)
- [ ] hide local Tarnished render
- [ ] first Limgrave collision passthrough
- [ ] composited SADX character render
- [ ] Seamless multiplayer state adapter

## Test without the games

The repository includes FakeSADX and FakeER so transport, target proxies and world-probe behavior can be tested without launching both games.

See `docs/SIMULATION.md`.

## Seamless Co-op

Seamless support is intentionally outside RingPass Core. The Elden Ring adapter will be designed to coexist with external DLL loading and avoid owning the game's networking layer.

## Safety / launch mode

Host-side development is for offline/modded Elden Ring sessions. RingPass is not intended to bypass Easy Anti-Cheat or operate in official online play.
