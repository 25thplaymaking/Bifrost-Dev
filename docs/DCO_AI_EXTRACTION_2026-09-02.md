# DCO AI extraction

## Preservation point

The complete pre-extraction implementation is preserved at commit `d59be46bd101bc7dab62deac15ee900fd5c0263f` on branch `bishop/dco-ai-compat-source-v1.0.28`.

That branch is the source boundary for a future optional AI compatibility addon. Bifrost's active release branch does not depend on that future package.

## Removed from Bifrost

The extraction removes 36 autonomous-AI script files plus their folder metadata, JSON configuration, global and per-entity editor attributes, and the obsolete AI-world override.

- Tactical movement: flanking, threat-funnel avoidance, exposure scoring, procedural paths, traveling overwatch, bounding overwatch, cover seeking, and standoff distance.
- Tactical decisions: the group tactical brain, cross-group coordinator, contact reactions, automatic formation changes, and formation-shape placement.
- Morale and suppression: group/member morale, panic, accuracy scaling, morale contagion, flee smoke, cover response, and armor avoidance.
- Specialist behavior: launcher discipline, machine-gunner positioning, emergency rearm, friendly-fire lane checks, vision limiting, idle behavior, and straggler merging.
- Autonomous stance behavior: cover stance, stance cooldown, global stance forcing, and persistent DCO stance locks.
- Global tuning: `DCO_Settings.example.json`, its loader, Base Settings, group/unit overrides, and the associated editor controls.
- Waypoint AI augmentation: the custom Tactical, Rush, Charge, DCO Flank, and DCO Covered approach modes.

## Retained Bifrost boundary

Bifrost retains features that are explicitly initiated and controlled by a Game Master:

- Native waypoint placement and completion-radius editing.
- Server-authoritative native stance, formation, combat-mode, and movement-speed commands.
- GM scenario authoring, triggers, staged groups, task zones, ambush/defend/QRF placement, and directed building clearing.
- AI animation posing, overlays, awareness visualization, and pause/freeze controls.
- Arsenal, effects, compositions, markers, world controls, and the rest of the GM interface.

The retained directed CQB helper now owns only the settings and geometry it needs. QRF no longer depends on morale or the tactical coordinator; it responds to nearby same-faction contact through its explicit GM designation.

## Retained feature review

- Formations are issued only by the server, apply to the native group formation component and every active native movement handler, and are periodically reasserted while moving. Standing formation correction excludes players and nested subgroup agents.
- Stance commands use native `SCR_AIStanceHandling.SetStance` on the server and affect only direct, AI-controlled members of the selected group.
- Ambush position and kill-zone pairing now has explicit arm, spring, rearm, range, and pairing diagnostics. Rearming reapplies hold fire, while an independent manual hold-fire order remains in force when an ambush springs or is disabled.
- Defence groups retain their area but can reorient when the nearest threat direction changes materially; contact loss and issued orientation changes are reported.
- QRF waypoints serialize as QRF rather than CQB, remain available as rally points during a response, retarget when the supported group changes, and report deployment and stand-down transitions.
- Directed CQB clearing reports engagement, completion, and failure states. The active building bounds are reconstructed and drawn by each Game Master client so the cue is available to remote GMs on dedicated servers.
- Completion radius is server-owned, replicated for join-in-progress, updates the native waypoint completion value, regenerates the native area mesh, and reports each accepted change.
- Task zones now use their role colors consistently: QRF blue, defence green, ambush purple, kill-zone red, and reinforce yellow.
- Missing Bifrost order or tactics widgets now produce explicit error diagnostics instead of silently leaving a button unbound.

## Resource and action audit

The post-extraction audit covers GUI layouts, editor prefabs, attribute configs, serialized DCO classes, deleted resource GUIDs, and the dynamically populated COMMAND menus.

- No layout, prefab, or config references an extracted class or deleted resource GUID.
- Every remaining serialized DCO type resolves to a live script class.
- The static COMMAND buttons remain because their stance, formation, behavior, tactics, native order, objective, spawn-point, and loop-order backends are still present.
- `Auto (release)` was removed from the Stance menu because persistent DCO stance locking was extracted; leaving the entry would expose a no-op action.
- The DCO context actions retained in `EditorModeEdit.et` map only to GM-directed hold/resume, ambush, and QRF actions that remain in Bifrost. The obsolete targetless CQB action was removed; directed clearing remains available through `COMMAND > TACTICS > Clear Building`, where the GM supplies the required building target.

## Compatibility-addon handoff

When the optional addon is created, start from the preservation branch and move the removed modules into a new addon identity. Its manifest should depend on Bifrost and Arma Reforger data. Reintroduce UI controls from the addon only where they enable one of its optional AI systems; do not make Bifrost require the addon.

Dedicated-server, remote-client, and join-in-progress acceptance must be run independently for the addon after extraction. Compile/static validation in Bifrost does not establish those runtime levels.
