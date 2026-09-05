# RPL and Visual Action Sweep Decision Record

## Requirements

- Replace every released GM-facing legacy diagnostic shape with the Bifrost canvas renderer.
- Render trigger shape, rotation, finite height and runtime state from replicated component properties.
- Render task-zone, tracer, strike, tracking, sound and mortar ranges from replicated placeable state.
- Keep gameplay mutations server-authoritative and keep presentation client-only and Game Master-only.
- Review the current composition, marker, world-control, briefing, ambient FX, vehicle-service, overlay and command paths for remote-client and join-in-progress design.

## Minimum components needed

- One rotated area/volume primitive in the existing GM render manager.
- Client registration for the existing replicated placeables.
- One render pass in the existing awareness-cue subscriber.
- Static, Workbench and multiplayer acceptance checks.

## Rejected or needs clarification before action

- No new service, dependency, replicated pixel state or second renderer.
- No release or Workshop publication is part of this sweep.
- Dedicated-server, remote-client and join-in-progress success require their explicit runtime pass and are not inferred from compilation.

## Primary risks

- Missing a placeable during client or join-in-progress registration.
- Exceeding the canvas command budget when many large areas are visible.
- Showing Game Master-only cues to ordinary players.

## Request interpretation

Every shipped GM action cue must be derived client-side from authoritative replicated state or an authority snapshot, rather than constructed as a local or server diagnostic shape.

## Understanding of the overall task

Finish the visual-path migration, close replication or authority gaps found in the current feature sweep, compile and statically validate the result, then preserve the dedicated-server, remote-client and join-in-progress boundary as an explicit pre-release acceptance gate.
