# First test checklist

Use the latest development release.

## 1. Infrastructure self-test

Open:

```text
Tools/RingPassSelfTest.exe
```

Expected:

```text
PASS
- SADX -> ER channel live
- ER -> SADX channel live
- protocol compatible
- ground probe received
- 3 target proxies received
```

If this fails, stop there and keep the console output.

## 2. Real SADX test

1. Copy `SADX-Mod` into the SADX `mods` folder.
2. Enable **RingPass SADX Bridge** in SA Mod Manager.
3. Launch SADX.
4. Enter gameplay with Sonic.
5. Open `Tools/RingPassMonitor.exe`.

Expected SADX side:

```text
[SADX -> ER] LIVE
Character: Sonic
Position: ...
Velocity: ...
Action: ...
Animation: ...
Grounded: YES/NO
Rings: ...
```

Run, jump and collect a ring. The values should change immediately.

The SADX log is stored inside the RingPass mod folder:

```text
logs/ringpass-sadx.log
```

## 3. SADX + simulated Elden Ring

Keep SADX open and start:

```text
Tools/FakeER.exe
```

The monitor should show:

```text
[ER -> SADX] LIVE / IN WORLD
Ground: HIT
Targets: 3
```

The SADX log should report that three host target proxies were prepared.

They are deliberately **not injected into SADX's native target list yet**.

## 4. Real Elden Ring telemetry test

Only use an offline/modded Elden Ring launch.

RingPass does not target official online play and does not include an Easy Anti-Cheat bypass.

The package includes an example Mod Engine 2 configuration. Put `RingPassER.dll` next to the Mod Engine 2 launcher and add it through `external_dlls`.

Start Elden Ring and enter the world, then run:

```text
Tools/RingPassMonitor.exe
```

Expected if current signatures match:

```text
[ER -> SADX] LIVE / IN WORLD
Player: X / Y / Z
Camera: X / Y / Z
Camera Q: X / Y / Z / W
FOV(rad): ...
```

For Elden Ring file version **2.7.1.0 / App Ver. 1.17.1**, the development bridge will additionally attempt:

- game-thread ground raycast
- nearby hostile character enumeration
- TargetProxy publication

Expected:

```text
Ground: HIT
Targets: <number of nearby hostile characters>
```

For any other executable version, those version-specific features remain disabled and the log explains why.

The Elden Ring log is:

```text
%LOCALAPPDATA%\RingPass\logs\ringpass-er.log
```

## What to send back after testing

The most useful results are:

- screenshot or copied text from `RingPassMonitor.exe`
- `ringpass-sadx.log`
- `ringpass-er.log` if the ER-side test was run
- whether SADX itself remained stable while running/jumping
- whether Elden Ring remained stable when the bridge DLL loaded
