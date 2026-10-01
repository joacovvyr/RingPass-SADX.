# Architecture

## Authority split

RingPass must not recreate SADX character gameplay unless there is no viable way to feed host-world data into the original SADX systems.

### SADX authority

- movement state machine
- acceleration and momentum
- jump/air states
- character animations
- Sonic Homing Attack selection/execution
- Gamma lock-on selection/execution
- Tails flight
- Knuckles glide/climb
- Amy hammer mechanics
- Big character-specific interactions

### Elden Ring authority

- world position space
- terrain
- collision source
- host enemies
- bosses
- host HP
- world streaming

## Target proxies

Elden Ring entities are represented inside the SADX side as lightweight target proxies.

The proxy contains enough information for SADX targeting code to make the decision itself:

- ID
- position
- aim point
- radius
- targetable
- alive

RingPass should **not** pick Sonic's Homing Attack target itself.
RingPass should **not** pick Gamma's lock-on targets itself.

It exposes candidates; SADX keeps the original selection rules.

## Collision plan

Phase 1 uses ground and wall probes.

Phase 2 builds a local collision representation around the active character from Elden Ring queries.

The aim is to preserve SADX slope/momentum behavior while sourcing surface data from Elden Ring.

## Seamless compatibility

Seamless support is intentionally separated from the core:

```
RingPass Core
  |
  +-- EldenRingAdapter
        |
        +-- Base Host Adapter
        +-- Seamless Adapter (future)
```

Do not make the core depend on Seamless-specific networking or ownership logic.
