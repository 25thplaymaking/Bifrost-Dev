# Release testing

Static checks and script compilation are performed before the stable tree is handed back. The following items still require an interactive game session before public release.

## Script budget gate

Before each stable sync, audit every method under `Scripts` and fail the release check when either limit is exceeded:

- more than 250 physical lines in one method;
- more than 300 estimated source-instruction units, counted as semicolons + call sites + twice the control-flow keywords.

The estimate is a conservative hotspot detector, not a substitute for Workbench compilation. Review methods above 225 units or 180 lines before adding more logic. Also record the total static declarations and initializer items so module-level growth is visible between releases.

The 2026-08-26 baseline is 1,931 methods, with a maximum of 294 units and 204 lines. The same scan found 569 static declarations and 737 initializer items.

## Replication gate

The 2026-08-26 baseline contains 60 replicated properties, 23 RPCs, and 11 explicit authority state bumps. Before each stable sync, verify that:

- every custom prefab containing replicated state has an `RplComponent` and remains available to all clients;
- Game Master attributes that write replicated state are server-controlled;
- client-to-server Game Master RPCs validate the caller on the server;
- owner RPCs originate on the authority, while broadcast RPCs carry transient presentation only;
- every authority-side runtime change to a replicated property bumps the owning entity;
- proxy-side previews cannot replace authority writes and must converge through a reliable server request plus replicated state.

The dedicated-server reconnect test must cover pause state, budget limits, active FX state, trigger latch state, and task-zone configuration. A reconnecting client must converge from replicated state without replaying transient effects.

1. Run a dedicated server with a remote Game Master and a second client. Reconnect the second client during the session and complete the replication gate above.
2. Open, close, and reopen every Bifrost surface. Confirm selection, search focus, camera control, dragging, resizing, and the reset-layout action.
3. Run cosmetic and LIVE helicopter gunruns. Confirm cosmetic tracers are visible and cause no damage; confirm LIVE rounds launch, trace, impact, and damage normally.
4. Exercise every tracer, explosion, mortar, flyby, gunrun, loiter, and custom-audio option that is exposed in the FX panels.
5. Exercise the full trigger activation and action matrix, including one-shot re-arming and paired actions.
6. Verify precise movement, rotation, snapping, attachment, simulation, visibility, poses, nametags, overlays, awareness cues, and audible-radius displays on host and remote client.
7. Verify pause-state actions, scenario preset save/load, time/date/weather controls, and interface visibility.
8. Exercise Hold Fire with two groups, launcher discipline against infantry and vehicles, machine-gunner positioning, morale, QRF, ambush, defend, standoff, and CQB clearing.
