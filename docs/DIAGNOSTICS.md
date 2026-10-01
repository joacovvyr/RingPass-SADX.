# RingPass diagnostics

The development tools classify each IPC direction independently.

Possible channel states:

- `LIVE`: heartbeat is current and protocol versions match.
- `STALE`: the process published data previously but stopped updating.
- `INCOMPATIBLE`: shared memory exists but the producer uses a different RingPass protocol version.
- `OFFLINE`: no heartbeat has ever been published.

The current heartbeat timeout is 2 seconds.

This distinction matters because SADX and Elden Ring are separate processes. A stale SADX channel must not be treated as valid character input by the Elden Ring bridge, and stale host collision/target data must not continue driving SADX.

## First real SADX test

Expected state:

```text
SADX bridge: LIVE
ER bridge:   OFFLINE
```

Then start `FakeER.exe`:

```text
SADX bridge: LIVE
ER bridge:   LIVE
```

If SADX is closed, its channel should transition from `LIVE` to `STALE` within roughly two seconds.
