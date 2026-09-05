# RPL and action-visual sweep — 2026-09-04

## Release decision

The current scripts pass the static replication review and Workbench compilation. Persistent Game Master action visuals now use the Bifrost canvas path and consume replicated entity state instead of local legacy `Shape` loops.

This is **not yet a multiplayer release clearance**. A dedicated server, remote Game Master, ordinary remote player, and reconnect/JIP pass must complete the acceptance cases in `docs/TESTING.md` before publishing.

## Scope and authority standard

- Review baseline: release commit `d59be46` through the current working tree.
- Game Master and player requests cross to the server through reliable server RPCs unless the data is deliberately transient telemetry.
- Gameplay mutations are validated and executed on the server.
- Persistent state used by clients is carried by `RplProp`, native replicated systems, or explicit snapshot RPCs.
- A visual is multiplayer-ready only when a remote client can reconstruct it from replicated state or an authoritative snapshot; a server-local debug shape is not evidence.
- Dedicated-server, remote-client, and JIP behavior remain separate runtime gates.

## Corrected findings

1. Trigger footprints still used the old world `Shape` renderer. Ellipse and rectangle footprints, horizontal rotation, unequal X/Z radii, finite-height bounds, and enabled/runtime-state colors now render through `DCO_GMRenderManager` from replicated trigger properties.
2. Trigger proxies returned before joining `DCO_TriggerRegistry`. Registration now occurs on every proxy, allowing remote and JIP Game Masters to discover existing triggers without reopening Properties.
3. Task zones, ambush range, tracer aim/sound, explosion and air-support marker/radii, mortar spread/sound, QRF range, and tactics placement previews had mixed local `Shape` paths. Persistent cues now share the canvas renderer; edit previews remain private to the editing Game Master.
4. Tracer emitter proxies were not registered for the awareness-cue pass. Registration and removal now occur on every proxy.
5. CQB clear maintained an always-on server `Shape` path alongside the replicated canvas cue. The duplicate legacy path was removed; the canvas cue remains driven by replicated group intent.
6. The only remaining `Shape.CreateLines` use is the short-lived gun-run tracer effect delivered by a broadcast cosmetic RPC. It is an in-world effect, not a persistent Game Master diagnostic or action-area cue.
7. LIVE loiter fire previously relied on the script-spawned projectile alone for its tracer presentation while native turret AI was intentionally disabled. LIVE and COSMETIC gunruns now share the broadcast tracer and caliber-matched sound path; the server alone spawns LIVE ammunition, and each pass snapshots its replicated armament selection.

## Replication matrix

| System | Server authority and transport | Remote/JIP reconstruction | Visual result | Runtime release gate |
| --- | --- | --- | --- | --- |
| Trigger conditions and actions | Server-only evaluation/action execution; replicated configuration and runtime state | Streamed trigger proxy plus `RplProp` state; registry now includes proxies | Canvas ellipse/rectangle, rotation, height, and state color | Activate/deactivate/rearm from a remote GM; reconnect and verify all cues and Once state |
| Task, ambush, defence, QRF, and CQB zones | Server-owned AI behavior; replicated placeable properties and group intent | Streamed placeables; CQB intent is replicated | Role/range circles and CQB building bounds on the GM canvas | Exercise behavior and visuals from remote GM and after JIP |
| Tracer, explosion, air support, loiter, and mortar | Server simulation; replicated settings; server-spawned LIVE ammunition; broadcast only for transient shot presentation | Streamed effect proxies and replicated settings; gunrun armament is snapshotted per pass | Aim, marker, scatter, tracking, spread, and sound ranges on the GM canvas; visible gunrun tracers and caliber-matched sound | Verify all six armaments in COSMETIC and LIVE remotely, confirm authoritative damage, then reconnect |
| QRF and tactics edit previews | Local edit state only; no gameplay mutation | Intentionally not shared | Canvas preview visible only to the editing GM | Confirm another client never sees the preview |
| Vehicle service | Reliable server requests/results; server validates caller, zone, target, and capability before timers or mutation | Authoritative vehicle state and access-anchor replication | Existing service UI; no server-local diagnostic dependency | Run all service modes remotely and after JIP; verify vehicle remains usable |
| Compositions | Server validates GM rights, content, hierarchy, transforms, and atomic placement; reliable owner/broadcast snapshots | Explicit library snapshot on initialization/open and authoritative server persistence | Client library reflects server snapshot | Remote capture/place/delete/undo, restart persistence, invalid rollback, and JIP sync |
| Markers | Server validates GM rights and mutation; reliable owner/broadcast snapshots plus native marker state | Snapshot requested during client initialization | Markers rebuild from server state | Remote create/edit/delete and reconnect/JIP |
| Mission briefing | Server/GM-gated request into the native replicated briefing component | Native replicated briefing entries | Journal content updates for all clients | Remote edit, non-GM rejection, live update, and JIP |
| World controls | Reliable server RPCs with GM-rights checks and authoritative entity resolution | Native entity state or `RplProp` state with replication bump where required | UI reflects authoritative results | Remote doors/lights/teleport/surrender/garrison and JIP state |
| Ambient FX | Server-owned replicated properties with change callbacks; headless instances do not build visuals | Each client rebuilds the effect from replicated properties | Client-local rendering from authoritative settings | Remote property edits, late stream-in, delete, and JIP |
| AI vision and navigation overlays | Server produces authority snapshots; unreliable transport is limited to replaceable high-frequency overlay data with serial/stale handling | Remote GM consumes current snapshots and replicated fallback | Private GM overlay; no gameplay authority | Remote selected/all modes, drag updates, loss recovery, and reconnect fallback |
| Arsenal, animation FX, orders, pause, time, budget, and garbage clear | Reliable server RPCs with rights or feature-specific validation; authoritative changes occur on the server | Replicated entity/native state and targeted result RPCs | UI state follows authoritative outcome | Run the corresponding dedicated/remote/JIP cases in `docs/TESTING.md` |

## Static evidence collected

- Enfusion replication behavior was checked in the live Workbench knowledge source, then against the BI Wiki multiplayer-scripting guidance, then against PAC1 game data for the legacy debug-shape and canvas primitives.
- Workbench script validation passed with zero Bifrost errors. The 14 emitted warnings are base-game deprecations.
- All 23 newly added runtime resources in the reviewed range have metadata.
- All 89 referenced Bifrost editor-attribute classes resolve to script declarations.
- No merge markers, zero-byte repository files, duplicate metadata GUIDs, or eager static object initializers were found.
- `git diff --check` passed; Git only reported the checkout's existing LF-to-CRLF conversion notices.

## Required runtime acceptance before release

Use one dedicated server with:

1. one remote Game Master;
2. one ordinary remote player to confirm Game Master cues remain private; and
3. a reconnect or separate JIP client after the entities and state already exist.

Complete the multiplayer acceptance pass in `docs/TESTING.md`, with particular attention to its trigger visual/state, all persistent action-visual, vehicle-service, compositions, briefing, navigation/vision overlay, CQB, world-control, Arsenal, and animation-FX cases. Record dedicated-server logs separately from what the remote and JIP clients observe. Do not promote the release if any authoritative mutation is client-only, any persistent cue depends on opening Properties, or any JIP client reconstructs incomplete state.
