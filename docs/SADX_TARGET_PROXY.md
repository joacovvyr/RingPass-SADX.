# SADX target proxy research

RingPass must preserve SADX's own targeting logic rather than selecting a target on the bridge side.

The current SADX Mod Loader exposes the same nearby-enemy structures used by the game:

- `around_enemy_list_p0`
- `ael_num0`
- `colaround { taskwk* twp; Float dist; }`

Public SADX multiplayer code also demonstrates that Sonic's homing routines consume this nearby-enemy list and resolve target positions through the target task's collision info.

## RingPass approach

Elden Ring publishes lightweight `TargetProxy` records:

```text
host entity id
position
aim point
radius
alive
targetable
```

The SADX bridge turns those into synthetic, local SADX task/collision objects.

Then the planned injection stage is:

```text
Elden Ring enemy
      |
 TargetProxy
      |
synthetic taskwk + colliwk + CCL_INFO
      |
around_enemy_list_p0
      |
original SADX targeting logic
      |
Sonic Homing Attack / Gamma lock-on
```

This preserves SADX's original angle/range/character checks.

## Current safety state

The synthetic-target builder is compiled and type-checked, but it is **not yet inserted into the live SADX enemy list**.

The insertion point must be validated in-game because SADX rebuilds/clears collision target lists during its frame collision analysis. Injecting at the wrong point would either be immediately erased or leave stale pointers.

Once the first SADX telemetry test succeeds, the next runtime experiment is to determine the correct post-collision-analysis injection callback/hook.
