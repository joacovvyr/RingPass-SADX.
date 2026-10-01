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

The synthetic-target builder and native list injection path are now implemented.

RingPass hooks SADX `CCL_Analyze` at `0x420700`, calls the original collision analysis first, then appends mapped synthetic targets to `around_enemy_list_p0` and updates `ael_num0`.

This preserves the list SADX itself just built and leaves the final target choice to the original character code.

The hook is **disabled by default** in `RingPass.ini`:

```ini
[Experimental]
InjectTargets=0
```

Before enabling it, validate:

1. base SADX telemetry,
2. ER/SADX coordinate mapping,
3. target proxy positions,
4. stability of the normal SADX collision loop.

The coordinate mapper anchors the current Elden Ring host-player position to the current SADX player position, then applies scale/axis settings before synthetic targets reach native SADX targeting.
