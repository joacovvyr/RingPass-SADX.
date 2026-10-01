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

## Initial targets

- Sonic
- Tails
- Knuckles
- Amy
- E-102 Gamma
- Big

## Architecture

```
SADX + SA Mod Loader
        |
  RingPassSADX.dll
        |
 Shared Memory IPC
        |
  RingPassER.dll
        |
    Elden Ring
```

## Milestone 0.1

- [x] repository skeleton
- [x] shared protocol definition
- [x] SADX bridge stub
- [x] Elden Ring bridge stub
- [ ] read active SADX character
- [ ] read character transform/state
- [ ] shared-memory transport
- [ ] read Elden Ring ground probe
- [ ] camera synchronization
- [ ] hide Tarnished proxy
- [ ] first collision passthrough

## Seamless Co-op

The project is intentionally structured so Seamless compatibility can be added later without making multiplayer hooks part of the core bridge.
