# What to send back after testing

If everything works, send:

- whether SADX launched normally with the mod enabled
- whether RingPassMonitor showed the SADX channel as LIVE
- whether FakeER made the ER channel LIVE
- the generated `RingPass-Capture.csv`

If anything fails, send:

- `RingPass-Diagnostic.txt`
- `RingPass-Capture.csv` if it was created
- `SADX-Mod/logs/ringpass-sadx.log`
- `%LOCALAPPDATA%/RingPass/logs/ringpass-er.log` if RingPassER was loaded
- a screenshot of RingPassMonitor if useful

Do not enable `InjectTargets=1` until the base telemetry tests have passed.

The useful failure categories are:

1. SADX mod does not load.
2. SADX bridge loads but IPC is OFFLINE/STALE.
3. SADX telemetry is LIVE but values are obviously wrong.
4. FakeER fails to produce ground/targets.
5. RingPassER does not load into Elden Ring.
6. RingPassER loads but reports unsupported game version/signature failure.
7. Player telemetry works but camera/ground/enemies do not.
8. Target proxies reach SADX but native target injection fails.

Each category can be debugged independently.
