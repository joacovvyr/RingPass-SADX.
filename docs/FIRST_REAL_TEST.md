# First real-machine test plan

Use the latest development release.

## 1. Offline package self-test

Run:

```text
Tools/RingPassSelfTest.exe
```

Expected result:

```text
PASS
- SADX -> ER channel live
- ER -> SADX channel live
- protocol compatible
- ground probe received
- 3 target proxies received
```

If this fails, run `RingPassDoctor.exe` and keep `RingPass-Diagnostic.txt`.

## 2. Real SADX telemetry

Install the contents of `SADX-Mod` as a normal SADX Mod Loader mod.

Keep this setting:

```ini
[Experimental]
InjectTargets=0
```

Do not enable native target injection yet.

Launch SADX, enter gameplay with Sonic and then run:

```text
Tools/RingPassMonitor.exe
```

Verify:

- SADX bridge is LIVE
- character is Sonic
- position changes while moving
- velocity changes while moving
- action/animation change during jump, run and homing states
- grounded changes in the air
- rings change when collecting rings

## 3. Record a useful telemetry sample

With Sonic in a stage, run:

```text
Tools/RingPassCapture.exe
```

For the next 15 seconds:

1. stand still for about 2 seconds
2. run forward
3. turn left and right
4. jump
5. perform a homing attack if a target is available
6. collect at least one ring

The tool writes:

```text
Tools/RingPass-Capture.csv
```

That file is intended to be shared back for analysis.

## 4. SADX + simulated Elden Ring

With real SADX still running, launch:

```text
Tools/FakeER.exe
```

Monitor should show:

```text
SADX -> ER: LIVE
ER -> SADX: IN WORLD / LIVE
Ground: HIT
Targets: 3
```

This validates real SADX against the simulated host side.

## 5. Diagnostic report

At any point run:

```text
Tools/RingPassDoctor.exe
```

It creates:

```text
Tools/RingPass-Diagnostic.txt
```

The report contains process presence, IPC health and a snapshot of both protocol channels.

## 6. Elden Ring test

Only after steps 1-4 pass should RingPassER be loaded into an offline/modded Elden Ring session.

The first Elden Ring test is telemetry only:

- bridge LIVE
- host state becomes IN WORLD
- player position changes
- camera position/FOV update
- ground probe reports hits
- target count changes near hostile enemies

Do not enable SADX native target injection until the host telemetry is validated.
