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
- The authoritative scale is stored in a replicated float. Its proxy callback applies scale. The current fallback RPC and custom RplLoad also write that float on clients. FRAME and POSTFRAME retain explicitly non-unit scales by writing only after scalar drift; returning to 1 removes event bits owned by Bifrost unless hidden presentation still needs them. The September 7 fallback retries unresolved targets every 250 ms for 40 attempts. There are no per-frame scale RPCs.
- The operation no longer calls Update or OnTransformReset recursively. It does not disable physics, change mass, rebuild collision geometry or replace characters with scenery.

## Operator acceptance

Reload the changed scripts or restart the play session before testing. These are source changes after published 1.0.29; the release tag and release fingerprint have not been rewritten.

1. Place a US medium barricade, a supply cache, a beehive and an AI rifleman. Select one, open Scale Object, enter **2**, and choose **APPLY SCALE**. Repeat at **0.5** and **1**. Confirm the result reports an updated target instead of a physics rejection.
2. Repeat for a vehicle, a GM-editable loose weapon/item and a player character. Check visible size, movement, stance changes, aiming, equipped items and collision. The player must be controlling a character, rather than spectating with no body.
3. Scale a whole assembly, then one attached part. Select both parent and child together and verify the child is covered by the parent instead of receiving a second multiplier. Move/rotate the assembly afterward.
4. Repeat from a remote GM on a dedicated server; observe from another client, reconnect for JIP and stream the entities out/in. Verify scale stays consistent after motion and that reset to 1 reaches all machines.
5. Capture/place a scaled composition. Verify persisted values, parts and hierarchy on the server and clients. Try an empty selection, a deleted target, invalid numbers, repeated IDs and a non-GM request; verify clear rejection and no unauthorized mutation.

## Verification boundary

Final Workbench WORKBENCH validation on September 4, 2026 passed with zero script errors and the same 14 existing base-game deprecation warnings. On September 6, dedicated-server feedback established that the requester saw the scale but ordinary clients did not. Installed source confirms native `SCR_EditableEntityComponent.SetTransform` deliberately preserves each peer's existing scale, so it cannot repair that gap. The current implementation keeps the rights-gated server mutation, adds a reliable player-controller broadcast for connected peers, and appends only explicitly set scales to the editable component's `RplSave`/`RplLoad` payload for JIP. The September 6 WORKBENCH validation passed with zero errors and 18 dependency/base-game warnings.

Workbench WORKBENCH validation is the compiler gate. The reliable broadcast and JIP serialization now exist in source, but the corrected path still needs a fresh dedicated-server observer and JIP run. No claim that arbitrary scaled characters retain fully correct animation, hitboxes or seat alignment is made from compilation or the API documentation alone.

## Documentation and replication audit — September 7, 2026

REQUIREMENTS
- Explain the reported observer mismatch using current source, session evidence and Bohemia documentation.
- Distinguish server mutation, state delivery, target resolution and rendered presentation.
MINIMUM COMPONENTS NEEDED
- Existing scale implementation, native source, existing logs and this document.
REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No further gameplay patch based only on a possible cause; no release or server restart.
PRIMARY RISKS
- Confusing a local scalar readback or RPC send count with successful remote presentation.
REQUEST INTERPRETATION
- This pass investigates scale; it does not certify the preceding retry as the root-cause fix.
UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Establish what is correct, what is fragile and what evidence identifies the failing stage.

The exact workspace and addon.gproj identity were checked. Workbench PID 26972 uses this workspace's addon.gproj, and the live MCP lists ArmaReforger and BifrostDev. Its state response has no active world path, so this audit supplies source/documentation evidence, not a new multiplayer run.

### Documented contract

- [BI Multiplayer Scripting](https://community.bistudio.com/wiki/Arma_Reforger:Multiplayer_Scripting), RplProp and streaming sections: authority changes plus BumpMe synchronize annotated properties. onRplName applies presentation on proxies, including streaming. Default RplProp group is Mandatory and condition is None. Consequently the scale annotation does not itself restrict delivery to GMs or the owner.
- The same document's RplSave/RplLoad example explicitly leaves an RplProp field out of manual serialization because it synchronizes automatically. Custom scale serialization is therefore additional machinery, not a documented prerequisite for JIP. The page warns that custom load versus member-update ordering must not be relied upon; its listed ordering is version-qualified.
- MCP's Enfusion “Replication overview” and “Replicating entities, components and hierarchies” explain that state follows target-node relevancy and streaming. An RPC on a player controller and the target component it references belong to potentially different nodes. A reliable RPC does not force an unrelated target to exist locally.
- [IEntity API](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfaceIEntity.html) exposes SetScale without a promise of network synchronization or arbitrary character/vehicle collision correctness. Update documents a separate transform-commit lifecycle for active entities. Reading GetScale immediately after setting it proves only that instance's scalar at that moment.

Installed native source corroborates the separation: SCR_EditableEntityComponent.SetTransformBroadcast saves the receiver's existing scale, assigns the incoming world transform, then restores the saved scale (native file lines 754–764). It cannot correct an observer that already has the wrong scale. The character movement path instead routes through SetTransformOwner/Teleport; replacing scale with a generic teleport is not a justified fix. The inspected native editable character and vehicle classes do not override RplSave/RplLoad or the frame callbacks; the player delegate serialization calls super.

### Current path and evidence

| Stage | Current implementation | What is established |
| --- | --- | --- |
| Request and authority | DCO_GMMissionServer.Apply checks GM rights; ApplyScale resolves delegates, removes duplicates and calls DCO_SetMissionScale | Server checks and immediate scalar validation are present |
| Durable state | DCO_GMMissionEntityState.c:51,97 stores scale and calls BumpMe; callback applies it locally | Matches the documented basic state pattern; no GM-only condition |
| Immediate fallback | DCO_GMMissionServer.c:469 sends one Owner RPC per player controller | recipients is an attempted-send count, without client acknowledgements |
| Target lookup | RPC resolves the editable component RplId; unresolved IDs retry 40 times at 250 ms | Covers about ten seconds of missing-target timing, not arbitrary future streaming |
| Stream initialization | DCO_GMMissionEntityState.c:128 adds custom scale payload beside RplProp | Redundant state transport; not proof that JIP was broken or is fixed |
| Presentation retention | DCO_GMMissionEntityState.c:283 checks scalar on FRAME and POSTFRAME | No delayed visual/bounds/physics measurements or per-peer retention results |

The existing client console logs_2026-09-06_18-55-02/console.log at lines 46988–46992 record target -2147479842 changing from 1 to 2, then receiving the fallback and applying 2 again. The first apply precedes the receive, consistent with the property callback already working on that client. This is not evidence for a failed observer. Resets to 1 also appear there. The negative printed identifier is not by itself an invalid-ID finding: the code uses RplId.IsValid and the logged client resolves that identifier successfully.

The previously reviewed dedicated-server log records requested=2, actual=2 and recipients=6. It identifies successful server mutation and attempts to notify six controllers. Neither that log nor the successful requester log establishes the failed observers' target existence, replicated value, later scalar or displayed size.

### Findings and next discriminating evidence

1. **The retry is a mitigation, not an established root-cause repair.** If an observer has no target for longer than ten seconds, the fallback expires; durable state must still recover when the target streams in. Extending the timeout alone would not explain a visible, already-resolved target staying small.
2. **There are three client writers for one field.** Property injection, DCO_ReceiveMissionScale and RplLoad can each assign m_fDCO_MissionScale. There is no revision linking fallback messages to target snapshots. This introduces an ordering risk across transport/lifecycle paths. No captured failed-observer trace currently proves an actual stale overwrite.
3. **The present success messages stop too early.** Server acceptance and immediate GetScale readback cannot detect later transform overwrite, inactive presentation updates, different parent transforms or visible geometry failing to follow the scalar.
4. **No source evidence supports a blanket “scale only replicates to GMs” rule.** The distinction in the reported outcome needs a failing peer's state and loaded artifact identity, rather than a new GM permission workaround.

Use one dedicated-server reproduction with a GM requester and an ordinary observer, holding the same target at 2 long enough to inspect it before resetting to 1. Record matching addon builds and prefab/component identity, target resolution, property callback versus RPC arrival, desired scale, immediate and later GetScale, event mask/active flags, parent/world transforms, and visible size. Repeat on a static prop and character, then reconnect and stream out/in.

- Missing target at receipt: investigate target streaming; verify state recovery after appearance.
- Present target but no current property value: investigate replication registration, node relevancy and artifact parity.
- Correct value initially, later wrong value: distinguish stale state assignment from a transform overwrite.
- Correct retained scalar but wrong visible size: investigate presentation/hierarchy and engine-specific geometry, not additional broadcasts.

The smallest documented long-term design is one authoritative replicated scale plus one local application path, with retention only where runtime evidence requires it. Any retained RPC fallback needs an explicit purpose and ordering contract. Simplification should follow the failed-observer trace so that removing a workaround does not hide the symptom. No gameplay source was changed in this audit.

### Supplied observer archives — reviewed September 7, 2026

Inputs: C:/Users/Bryce/Downloads/logs_2026-09-06_18-31-07.zip and logs_2026-09-06_20-13-35.zip. Read archive members directly without extracting or executing them. The earlier-named ZIP also contains the later session; SHA-256 comparison confirms the later console/error/script logs are identical across the two ZIPs. Counts below use each unique console log once, without double-counting mirrored script/error/crash entries.

The client downloaded Bifrost GUID 6A0C2D6CE9809C6E from version 1.0.30 to 1.0.31 before connecting. Its package is mounted as BifrostDev. This proves the reported version transition and loaded identity, not source-hash parity. Both sessions identify the 25th Armored Division - Operations Server.

**Clock alignment changes the interpretation.** This client's log header gives local 18:31:07 = 23:31:07 UTC (UTC-5). Bryce's header gives local 18:55:02 = 22:55:02 UTC (UTC-4). The ZIP names must not be compared directly to Bryce's local clock.

| Event | UTC | Bryce local time |
| --- | --- | --- |
| Server recorded scale 2, same target -2147479842 | Sep 6 23:24:15 | Sep 6 19:24:15 |
| Server recorded resets to 1 | Sep 6 23:24:26 | Sep 6 19:24:26 |
| Supplied client connected | Sep 6 23:31:51 | Sep 6 19:31:51 |
| Supplied client loaded/applied scale 1 | Sep 6 23:32:31 | Sep 6 19:32:31 |
| Supplied client disconnected, then reconnected | Sep 7 00:31:32 / 00:33:05 | Sep 6 20:31:32 / 20:33:05 |
| Second archived session connected/disconnected | Sep 7 01:14:49 / 01:16:53 | Sep 6 21:14:49 / 21:16:53 |

Server times above are Docker receipt timestamps and may be buffered by seconds. That does not account for the roughly seven-minute gap before this client connects. A fresh server read covering 160,003 output lines from Sep 6 22:12 UTC through Sep 7 05:29 UTC found only the previously recorded scale 2 and two scale 1 requests.

The earlier console at lines 6021–6023 (script.log lines 480–482) records:
- gm.scale.jip target=-2147479842 value=1
- gm.scale.apply target=-2147479842 requested=1 before=1
- gm.scale.result target=-2147479842 actual=1

There is no scale 2 receipt/application in either archived session. This absence does **not** demonstrate dropped replication: their captured connections start after the scale was reset. The JIP record proves that custom scale loading and target resolution ran on this additional client for the reset state. Since the model was already at 1, it does not prove non-unit visual scaling or property-only JIP. It also means the earlier “no additional client logs” evidence boundary is superseded; the remaining gap is an additional client log overlapping the non-unit scale operation.

Other actionable evidence:
- Earlier unique console: 4,614 RHS_RadioSourceActiveComponent exceptions, NULL m_Owner in Register, RHS_RadioSourceComponent.c:91; busiest recorded minute has 677 exceptions.
- 84 SAL_DroneControllerComponent destructor null-pointer exceptions at SAL_DroneControllerComponent.c:721.
- 56 Bacon GunBuilderUI_PreviewUIComponent exceptions over about 1.4 seconds: Entity.SetAngles() has NAN angles, preview Update:515 via M4Test_GunBuilder.OnMenuUpdate:334.
- 95 invalid-prefab replication insertion errors across both consoles: 68 for SCANNER_SIGINT_antenna.et (E2C127E61A2F95AB) and 27 for Structures/Wreck/volha.et (BD1F07D0C32629AF). These errors name specific resources and are not evidence that the successfully resolved scale target was invalid.
- Later unique console: eight entity-creation failures involving US spawn point, US/USMC arsenal boxes and F22 flyby, plus two duplicate MapMenu identifier errors. There are no VM exceptions in that later console.
- The archive's crash.log contains repeated VM exception reports. Its existence alone does not prove a fatal process crash; neither console contains an access-violation or “Resources are leaking” marker.

The repeated exception bursts are credible candidates for client hitches, but these logs have no frame-time correlation that proves how much lag each caused. No exception stack above points directly into Bifrost scale code. The new evidence strengthens the mod-error diagnosis and confirms reset-state JIP on one more client; it does not identify the original non-unit observer failure.
