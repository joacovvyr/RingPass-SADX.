# Passthrough integration design

Public reference: [Universal Modder Minecraft/GTA example](https://github.com/rehan-remade/universal-modder/tree/15d6f9d/examples/minecraft-gta5-passthrough).

## Reusable components

- HostLink.java and ws.cpp: separate control/events from frame payloads. Keep RingPass binary shared-memory telemetry; introduce acknowledged event IDs before attacks or damage.
- FrameExporter.java: asynchronous readback ring, three shared-memory slots, capture pose and echoed host-frame ID. Rewrite the capture backend for SADX; the Java/Fabric implementation is not directly usable.
- HostState.beginFrame(): freeze one host pose for all capture hooks, rather than reading changing telemetry between passes.
- compositor.cpp: texture bindings, resolution recreation and pose history. Adapt and validate the host graphics backend independently.
- MCPassthrough.fx: depth linearization, occlusion and camera reprojection. Calibrate units, axes, clip range, reversed Z, row orientation, aspect and vertical FOV for each adapter; its Minecraft assumptions are not universal.
- fakegta.cpp and fakehost.py: build synthetic occlusion and camera-motion tests before in-game composition.

## RingPass requirements

Use a separate versioned visual mapping alongside the existing telemetry mapping. Descriptors need fixed-width fields, dimensions, pitch, format, depth convention, near/far, capture pose, host-frame ID, process epoch and monotonic timestamps. Never transmit process pointers across x86/x64.

Start with isolated character color/mask plus depth, and a separate screen-space HUD. Normal full-game color is not automatically transparent. Verify depth accessibility before selecting readback or a dedicated geometry pass.

The reference uses CPU shared memory after GPU readback, followed by texture upload, rather than direct shared GPU textures. At 1080p three four-byte layers total approximately 24.9 MB per frame. Start with a lower resolution and measure bandwidth and latency.

Copy descriptors and pixels into private CPU staging, validate a stable sequence, then upload. The reference uploads before its final sequence check, so a rewritten slot may already have contaminated textures. Validate buffer sizes and multiplication overflow. Use aligned atomic sequence operations suitable for a 32-bit producer; a 64-bit field alone does not guarantee atomic publication.

Discard stale frames, reset on process/zone epoch changes, keep a bounded pose history, and measure reprojection against capture-time camera matrices. Reprojection does not synchronize physics or fix disocclusion.

Load RingPassER as an external DLL in a dedicated offline ModEngine profile. Preserve existing loader DLLs, mod configurations and saves. Test the base adapter before adding other mods; compatibility requires runtime testing. The SADX adapter requires the 2004 US environment supported by [SADX Mod Loader](https://github.com/X-Hax/sadx-mod-loader#system-requirements).

Keep visual composition and experimental target injection disabled until telemetry, synthetic occlusion and actual camera/depth calibration pass. Generic function prologues must resolve uniquely before any game function is called. Reject ambiguous signatures rather than selecting a candidate by scan order.

No Universal Modder implementation code was imported. Preserve its MIT notice if substantial code is adapted later.
