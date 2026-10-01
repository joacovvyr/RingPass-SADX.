# Offline bridge simulation

RingPass includes two simulator executables so the IPC and coordinate/target pipeline can be developed without both games running.

## FakeSADX

Publishes a simulated Sonic state at roughly 60 Hz.

It exercises the same SADX -> Elden Ring IPC channel used by RingPassSADX.dll.

## FakeER

Reads the SADX channel and publishes:

- an in-world host state
- a flat ground probe
- three moving target proxies

It exercises the same Elden Ring -> SADX IPC channel reserved for RingPassER.dll.

## Monitor

Run RingPassMonitor alongside either simulator.

A complete simulation test is:

1. Start FakeSADX.
2. Start FakeER.
3. Start RingPassMonitor.
4. Confirm both channels advance frames.
5. Confirm ground is HIT.
6. Confirm Targets reports 3.

This does not emulate Elden Ring. It validates transport and data ownership before game-specific reverse engineering is connected.
