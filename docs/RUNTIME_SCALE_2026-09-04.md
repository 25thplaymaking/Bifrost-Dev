# Runtime scale correction

REQUIREMENTS
- Allow runtime scale changes on every GM-editable entity category, including characters, vehicles, equipment, structures and compositions.
- Retain selected-target identity, server authorization, replicated scale and join-in-progress initialization.
- Preserve movement, animation, simulation and inventory; do not substitute a static visual clone.
- Report an actual rejected or unretained operation, rather than claiming that zero targets were scaled successfully.
- Leave interactive testing to the user and preserve the published 1.0.29 tag.

MINIMUM COMPONENTS NEEDED
- Existing mission panel, server request and editable entity's replicated scale field.
- A post-frame check only on explicitly resized entities, to restore scale if native animation or movement overwrites it.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- Remove the blanket physics, hierarchy and entity-category bans; they are Bifrost policy, not a restriction documented on IEntity.SetScale.
- No physics-body reconstruction, simulation disabling, static replacements, new networking system or automatic release.

PRIMARY RISKS
- Engine-specific animation, collision, seats, equipment and mod scripts can respond differently to non-unit scale. Native API availability and scalar readback alone do not establish correct gameplay in all these cases.
- Static transform reset/update is inappropriate for animated character controllers. Use the native scale setter without resetting their transforms.
- Parent and child selections must not be applied twice as nested assemblies.

REQUEST INTERPRETATION
- The user explicitly requests runtime scaling of all items, including characters, superseding the previous static-only policy. The existing GM selection and targeting interface defines reachable targets.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Remove Bifrost's unsupported restrictions, implement and validate the native replicated scale operation, and document the remaining runtime verification boundary honestly.

## Diagnostic evidence

The fresh Workbench log `logs_2026-09-04_22-58-07/script.log` records the US medium barricade at 23:01:20 followed by `resized 0; skipped 1` with the moving-physics rejection at 23:01:31. The request reached the server; the physics predicate stopped it before SetScale. The source sweep also found category and parent restrictions, no scalar readback, and a recursive static transform reset being used for every accepted target.

## Documentation consulted

- Enfusion MCP entity lifecycle, replication, IEntity and Physics references.
- [BI Game Master entity properties](https://community.bistudio.com/wiki/Arma_Reforger:Game_Master:_Entity_Property_Creation): replicated targets and server-side writes.
- [IEntity API](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfaceIEntity.html): SetScale/GetScale and transform lifecycle; no category ban is documented on SetScale.
- [Physics API](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfacePhysics.html): distinguishes transforms including scale from internal transforms without scale; it does not promise character-controller or vehicle behavior at arbitrary scales.
- Native PAC1CLI source: SCR_ScenarioFrameworkActionSetEntityScale calls IEntity.SetScale directly. SCR_EditableEntityComponent preserves existing scale during ordinary transform broadcasts; its static transform update is separate from the character owner teleport path. SCR_EditorLinkComponent creates composition parts under the root through native entity parenting.
- Broader search results about DayZ or Arma 3 were not used as evidence for Reforger behavior. The related BI procedural-animation issue could not be opened and was not treated as a confirmed entity-scaling limit.

## Implementation

- The only target requirement is a live replicated GM-editable entity. The existing GM server rights check, request throttle and 64-selection cap remain. Duplicate IDs and player delegates resolving to the same character are processed once. Native physical parenting covers nested selected parts without scaling them twice.
- Both panel and server use the same 0.01–100 range check; invalid/NaN/infinite values fail it. A single selection shows its current scale. Player selections show and mutate the controlled character's scale.
- The native setter is followed by scalar readback. If it does not accept the requested value, the old scale is restored and the response reports the rejection. Readback is not a test of collision geometry or animation correctness.
- The existing replicated float remains server-owned. Its proxy/JIP callback applies scale and enables a post-frame check only for explicitly resized entities. That check writes only after scalar drift. Returning to 1 removes only the post-frame mask added by Bifrost; no timers, global scans or per-frame RPCs are added.
- The operation no longer calls Update or OnTransformReset recursively. It does not disable physics, change mass, rebuild collision geometry or replace characters with scenery.

## Operator acceptance

Reload the changed scripts or restart the play session before testing. These are source changes after published 1.0.29; the release tag and release fingerprint have not been rewritten.

1. Place a US medium barricade, a supply cache, a beehive and an AI rifleman. Select one, open Scale Object, enter **2**, and choose **APPLY SCALE**. Repeat at **0.5** and **1**. Confirm the result reports an updated target instead of a physics rejection.
2. Repeat for a vehicle, a GM-editable loose weapon/item and a player character. Check visible size, movement, stance changes, aiming, equipped items and collision. The player must be controlling a character, rather than spectating with no body.
3. Scale a whole assembly, then one attached part. Select both parent and child together and verify the child is covered by the parent instead of receiving a second multiplier. Move/rotate the assembly afterward.
4. Repeat from a remote GM on a dedicated server; observe from another client, reconnect for JIP and stream the entities out/in. Verify scale stays consistent after motion and that reset to 1 reaches all machines.
5. Capture/place a scaled composition. Verify persisted values, parts and hierarchy on the server and clients. Try an empty selection, a deleted target, invalid numbers, repeated IDs and a non-GM request; verify clear rejection and no unauthorized mutation.

## Verification boundary

Final Workbench WORKBENCH validation on September 4, 2026 passed with zero script errors and the same 14 existing base-game deprecation warnings. Whitespace validation passed. Source review confirmed that scale requests still pass through the server rights gate and reliable controller request, while the server-owned replicated float drives local application and retention on proxies/JIP. No computer use, live reload, playtest or additional process was launched.

Workbench WORKBENCH validation is the compiler gate. Interactive, dedicated-server, remote-client and JIP results are separate and remain pending with the operator. No claim that arbitrary scaled characters retain fully correct animation, hitboxes or seat alignment is made from compilation or the API documentation alone.
