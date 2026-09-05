# Loiter Gunrun Decision Record

## Requirements

- Restore visible, audible gunfire from Armed Orbit while it engages the selected target area.
- Let the Game Master choose the gunrun ammunition/armament class.
- Keep firing, damage, target selection, ammunition use, and aircraft lifecycle server-authoritative.
- Make the selected ammunition replicate and remain stable for an already-started pass.
- Preserve support for cataloged vanilla and modded helicopters.
- Verify compilation and define separate dedicated-server, remote-client, and JIP acceptance evidence.

## Minimum components needed

- Reuse the six existing Bifrost tracer-ammunition definitions rather than add another catalog.
- Add one replicated gunrun-round property and one existing-style Game Master attribute.
- Snapshot the selected round into each aircraft pass.
- Broadcast the short-lived tracer presentation for both cosmetic and LIVE shots while spawning damaging ammunition only on the server.

## Rejected or needs clarification before action

- Do not re-enable autonomous turret AI: it would select its own targets and could double-fire alongside the deterministic gunrun.
- Do not rewrite helicopter flight, orbit, or catalog discovery; the fault is isolated to the firing/presentation path.
- Do not infer dedicated-server, remote-client, or JIP success from compilation.

## Primary risks

- A live projectile can fail to load, spawn, or launch while the cosmetic cue still appears.
- Changing the property during an active orbit could otherwise mix ammunition within one pass.
- An unreliable tracer cue may occasionally be dropped; repeated gunfire remains suitable for that channel, while damage stays authoritative.

## Request interpretation

“Armament/bullet type” means the ammunition class used by Bifrost's controlled gunrun, independent of whichever weapon happens to be mounted on a selected helicopter prefab.

## Understanding of the overall task

Repair the complete loiter gunrun so Armed Orbit visibly and audibly fires into its chosen area, provide a replicated caliber selector, preserve server-owned damage and target selection, and leave explicit multiplayer runtime acceptance before release.
