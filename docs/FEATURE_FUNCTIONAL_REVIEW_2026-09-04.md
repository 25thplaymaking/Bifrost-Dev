# Feature functional and layout review

REQUIREMENTS
- Review the new mission tools and the changed 1.0.29 feature groups from their visible controls through target selection, native actions and server execution.
- Compare the installed native implementation using PAC1CLI after MCP and BI documentation; check layout bindings as well as script compilation.
- Fix demonstrated wiring, target-resolution and lifecycle failures. Preserve the user's ownership of in-game testing.

MINIMUM COMPONENTS NEEDED
- Existing handlers, mission server and composition placement code receive local corrections.
- The existing tracer component separates non-damaging presentation from authoritative live ammunition, with one transient client cue and a broadcast per shot; no new service or dependency.
- A reusable source/layout integrity check and this feature-by-feature interaction ledger provide reviewable evidence.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No replacement UI framework, new dependencies, live reload, playtest automation or publication.
- Source inspection cannot prove engine physics, rendering, remote transport or join-in-progress behavior.

PRIMARY RISKS
- Player delegates are editor proxies rather than the character to modify.
- Bare widget handlers can accept unintended mouse buttons.
- A partially spawned entity must be registered for rollback before validating its components.
- Native APIs and authored layout controls must agree; successful compilation alone does not establish that agreement.

REQUEST INTERPRETATION
- Audit the released additions and the current runtime-scale change, including required clicks and selections; repair concrete defects, and give explicit functional boundaries.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Make each exposed feature's path usable and traceable from its layout to its authoritative outcome, with installed native-source evidence and an honest testing boundary.

## Findings repaired

1. Mission, composition, marker, CREATE, context, options, scenario, orders, tactics and vehicle-service action handlers accepted non-primary mouse clicks. They now reject those clicks before dispatch. This includes destructive composition deletion and immediate service actions. Explicit secondary-click inventory behavior remains intact.
2. Make Invincible resolved a Player-tab delegate's owner instead of its controlled character. It now shares the scale action's native delegate resolution and counts each actual damage target once, including overlaps between selected crew and selected vehicles.
3. Chatter put keyboard focus on Apply despite requiring a message. Its visible message box now receives initial focus.
4. Composition rollback registered each spawned object only after validating its editable component. A spawned object without that component escaped rollback. Registration now precedes validation. Stored composition validation also checks every item before following parent references, preventing a null parent item from being dereferenced during library loading.
5. Standalone FX Tracer cosmetic mode launched native bullet prefabs anyway. Installed `Ammo_Bullet_Base.et` contains `ProjectileDamage`; omitting `BaseTriggerComponent.SetLive()` does not remove these authored bullet effects. Cosmetic mode now returns before spawning ammunition. LIVE retains the authoritative projectile. Both modes send a transient presentation RPC: positional shot audio and an optional depth-tested tracer cue clipped at the first obstruction. Density controls which shots draw a cue; sounds still follow each shot when enabled. One cue per emitter is retained for 40 ms and cleared during teardown. This is a brief 80 m maximum visual cue, not a simulated cosmetic ballistic projectile. Packet loss may omit individual transient cues; ongoing firing state remains replicated.
6. Vehicle rocket rearm dereferenced a failed resource load. It now skips missing/invalid authored rocket resources without dereferencing null.
7. LZ/RP/Target help explicitly describes named coordinates, so the menu no longer suggests that saving one produces an AI landing pad or laser target.

## Mission actions: exact interaction paths

All 13 actions were traced through catalog/context dispatch, `DCO_GMMissionPanel`, `DCO_GMPlacementConfirm`, the mission server, relevant state/helper components and their resource registration. The shared panel's buttons and editable field types match `DCO_GMMissionTools.layout`. Setup entries are actions rather than prefab placement; Hide Terrain Objects uses a real native prefab.

| Feature | What the user must do | Code path and functional boundary |
| --- | --- | --- |
| Hide Terrain Objects | CREATE > Lightning > Bifrost; select Hide Terrain Objects, then click ground. Select the resulting system and open Edit Properties to change Radius or Hide objects. | Native placing component spawns `E_DCO_TerrainArea.et`. Server attributes use a float slider and bool checkbox. Replicated radius/enabled state drives each peer's hide claims and circle. Move, shrink, disable or delete releases old claims; overlapping areas retain their claims. Excludes GM-editable objects, characters and vehicles. Does not alter terrain height or baked navigation. |
| Restore Hidden Terrain | Select the setup action, then confirm with its Apply button; no target required. | Authority deletes all Bifrost terrain-area helpers; deletion releases claims on peers. This restores all such areas, not an individual selection. |
| Scale Object | Select GM-editable objects/characters first, or select Scale Object then click an editable world object. Enter 0.01â€“100 and Apply Scale; 1 resets. | IDs resolve on the server; Player-tab delegates resolve to their character. Native SetScale readback, replicated scale and post-frame retention. Selected physical descendants inherit their selected parent's scale operation. Character animation/collision and vehicle simulation still need actual runtime acceptance; source validation does not establish unrestricted engine physics scaling. |
| Make Invincible | Select damage-capable targets or player entries, or click a target after selecting the action. Choose ON/OFF, optionally current crew, then Apply Damage Setting. | Server applies native damage handling and replicated state. Delegate resolution/counting repaired. Does not heal existing damage or automatically protect future occupants. |
| Create/Edit Intel | Select one supported prop, open the action, enter title and message, choose audience and optional clue removal, Save Intel. Player approaches the prop and uses Read Intel. | Server-owned helper, native interaction collection and map journal. Private contents are returned only to authorized GM editing requests; awards go to authorized audiences and can be requested again after joining. Props must satisfy CanBind; this does not spawn a clue prop automatically. |
| Global Hint | Open setup, enter optional title and required message, Send Hint. No object click required. | Reliable owner messages to current players invoke native custom hints. Transient; joining later does not replay it. |
| Chatter | Select one living conscious AI, or clear selection for faction HQ. Enter text, choose audience, Send Chatter. | Server validates speaker and audience; native chat feed receives an explicitly marked AI message. Message box now starts focused. Text chat, not voice synthesis. |
| Create Teleporter | Select one supported prop, name the endpoint and link, Save Endpoint. Repeat on a second prop with exactly the same link name. Player approaches either prop and uses Travel. | Exactly two endpoints per link. Server validates living on-foot user, proximity/line of sight, clear destination, water/terrain limits and cooldown. No automatic prop creation or multi-destination menu. Unloaded remote target data can delay local destination presentation until streaming resolves it; authority rechecks the actual pair. |
| Use Named Position | Select supported AI groups or task/air/mortar modules, open setup, choose a saved position using Previous/Next, then Use This Position. | Server resolves the saved ID. Groups receive a native Move waypoint; supported modules use native editable SetTransform. Active strike snapshots keep their existing target. Deleted destination IDs disable Apply until another is chosen. |
| Remove Intel / Teleporter | Select the source props, open setup, Remove Interaction. | Server removes matching helper entities while preserving source props. The remaining endpoint becomes unpaired. |
| Create LZ | Select setup, click ground, enter a name, create. | Creates kind LZ in the replicated server marker library, consumable by Use Named Position. No AI helipad or selectable LZ world system. |
| Create RP | Select setup, click ground, enter a name, create. | Creates kind RP in the marker library. Does not itself spawn reinforcements. |
| Create Target | Select setup, click ground, enter a name, create. | Creates kind Target in the marker library. Does not designate a laser target. |

Escape cancels pending terrain/object targeting. Setup panels are modal, and clicking the CREATE/edit/top/context panels is excluded from world confirmation. Catalog object targeting uses the clicked editable entity rather than a stale selection. Context actions may operate on the current selected set. A bare terrain mesh has no editable target for Scale/Intel/Invincible.

## Other changed feature groups

These checks cover feature entry points and their downstream contracts, not every line of the entire addon. No additional wiring defect was identified in rows without a repair noted below. All rows retain the runtime boundary described later.

| Feature | Required clicks and configuration | Source/native contract reviewed |
| --- | --- | --- |
| Composition capture, library, place, undo, delete | Select objects, right-click > Save Selection as Composition; fill the labeled name/category/author fields and Capture. Select a library row, Place in World, then click terrain. Undo removes your last placed set; Delete removes the saved entry. | Separate composition layout, eight generated rows, bounded scroll, native editable IDs/hierarchy and server capture stream. Profile JSON persistence and rollback paths reviewed/repaired. No CREATE composition item. Saved prefab transforms are not a full mission-state snapshot. |
| Gunsmith weapon carousel | Open Arsenal > Gunsmith; click the receiver card, choose weapon category, then click a tile. Prev/Next pages the strip. Wear applies the draft. | Row uses native button activation for mouse/menu-select; stage drag excludes UI controls and mouse focus does not move tiles before release. Receiver selection calls SetDraftWeapon and the draft event refreshes the stage. Weapon/tile/receiver layouts and bindings inspected. |
| Gunsmith attachments | Click an attachment slot chip to open compatible candidates; click a candidate to mount it, or the mounted candidate to remove it. Open Contents manages stored gear. | Native slot enumeration, slot types and storage validation. Candidate click reaches SwapDraftAttachment; incompatible preview placement restores the previous draft attachment. The unstable position/remount control is intentionally absent. Magazine chip adds compatible magazines to the selected inventory container, rather than changing the displayed loaded magazine. |
| Soldier gear and contents | Select a gear category/card; open its contents. Back/Escape returns to the originating category; another card replaces that category. | Native inventory-slot/storage APIs, category restore cancellation and draft deposits. Harness-owned pouch storage remains the deposit target. |
| Kits and Arsenal settings | Select a kit to wear it; focus/hover selects the target of rename/overwrite/delete. Save Current saves the draft. Settings use authored spinbox/slider components. | Native button activation, shell apply-result routing and settings layout bindings. KitsStageWorld intentionally uses the explicit runtime render fallback attached to KitsStage. |
| Arsenal Access | CREATE > Lightning > Bifrost > Arsenal Access, then click a supported editable object. Approach/aim at its interaction point and use Arsenal. | Top-level helper with native replication snapshot, target identity/anchor, manual interaction collection and server proximity/policy validation. Native object actions remain collected. |
| Vehicle Service Bay | Place the bay, bring a supported vehicle within its area, approach the service interaction, select a supported operation and wait for server-authorized progress. | Repair/refuel/rearm capability mask controls buttons and durations. Server validates again at completion; rearm only refills authored mounted systems. Native fuel/inventory/muzzle APIs and reliable start/result RPCs inspected. Right-click handling and failed rocket load guard repaired. |
| Mission Information briefing | Open Mission Information, select Situation/Mission/Execution/Signal/Intel, edit the text box, Save. | Widget bindings and native RewriteEntry_SA. Native briefing component broadcasts changes and serializes them in RplSave/RplLoad. Requires that component on the active game mode. |
| Change Side Relations | Context menu > relation source faction > target faction > Friendly/Hostile. | Server GM rights check; native faction-manager methods update relationships and AI targets. Native faction snapshots serialize relation state. |
| Suppressive Fire | Select an AI group, Orders > waypoints > Suppressive Fire, then click the desired suppression area. | Native editable suppression waypoint is present in the installed archive; native placement recipients connect it to selected groups. Native suppression activity starts when the waypoint becomes current, not just when its icon is selected. |
| Place Comment and named marker editing | Right-click ground > Place Comment; choose local/server scope, fill text/settings and save. Select entries in the marker editor to edit/remove. | Marker layout, generated handlers, local state versus rights-checked server mutations and replacement snapshots inspected. Server comments share state with GMs; local scope intentionally remains local. |
| Teleport Players | Mark selected Player-tab entries for teleport, right-click clear ground and choose Teleport Players Here. | Client computes spaced destinations; server validates rights/player/bounds and invokes native server player teleport. No teleport on merely selecting a player. |
| Stance and formation | Select AI group(s), Orders > Stance/Formation, choose the value. | Authority invokes native character stance handling and active group movement-handler formations. Direct AI members only; no player control or removed autonomous AI suite. Native movement API documents -1 as the absent-handler sentinel. |
| QRF, ambush, defend and reinforcement zones | Place the appropriate system, select it and edit properties. Use the tactics controls to assign/send groups; configure range, pairing and arming as appropriate. | Task-zone server ticks/assignment, QRF waypoint intent, ambush hold-fire/rearm and tactics widget bindings inspected. Zone placement alone does not conjure a group. Native groups/waypoints carry movement. |
| Directed CQB | Select group(s), COMMAND > TACTICS > Clear Building; choose a suitable building target. | Building survey/targeted waypoint path and remote bounds presentation inspected. Requires a usable building/interior; the removed targetless CQB toggle remains absent. |
| Waypoint completion radius | Select a waypoint, Edit Properties > Completion Radius; adjust it. | Native SetCompletionRadius plus replicated mirror and area-mesh regeneration. Float attribute registration matches the editor control. |
| Triggers and synchronization | Place GM Trigger, configure Setup and Units, link/synchronize intended entities, review Finalize and arm it. | Native editor attribute registration, owner/area conditions, server trigger tick, staged groups and sync binding lifecycle. Replicated shape/rotation/height/runtime state feeds GM canvas cues. Merely placing an unarmed trigger does not fire its response. |
| Air support/Loiter gunruns | Place the module, choose aircraft/armament/LIVE or cosmetic, configure pass/orbit settings and activate it. | Server projectile launch and snapshotted round type; transient broadcast shot/tracer cues, native ammunition and sound resources. LIVE can damage; cosmetic does not spawn the damaging projectile. |
| FX Tracer | Place and rotate the emitter, select its native properties, choose round/rate/burst/density/LIVE/sound and start firing. | Repaired cosmetic/live split and remote presentation. Replicated ongoing settings; shot cues are transient. Native bullet effects inspected down to Ammo_Bullet_Base.et. |
| FX: Emitter | Place emitter, select native properties, choose preset/scale/enabled. | Existing replicated properties rebuild peer-local particle effects. Rename preserves the resource GUID. This visual scale is separate from Scale Object. |
| Persistent GM visual cues | Place/configure the owning trigger/zone/module; enable relevant GM overlay visibility. | Client canvas draws from replicated owning state, including trigger rotation and height. Draw cues do not execute gameplay actions; non-GMs should not receive the GM overlay. |
| CREATE scroll, names, faction folders and accents | Expand a folder, drag scrollbar track/thumb or use wheel; hover truncated names for full text. Choose a color swatch, Black or White in options. | Track/thumbnail binding, DPI conversion, release/focus-lost cleanup, row pool and fixed 18-pixel ellipsis inspected. Collapsed initial tree and session expansion state remain. Accent mode is persisted independently of hue. |

## Native evidence consulted through PAC1CLI

MCP knowledge was consulted first, followed by the full BI [Game Master Entity Property Creation](https://community.bistudio.com/wiki/Arma_Reforger:Game_Master:_Entity_Property_Creation) documentation. The installed game's native source was then read using `C:/Users/Bryce/pac1-cli/pac1cli.exe`. Archive entries are evidence; no native implementation was copied into the addon.

From `P:/SteamLibrary/steamapps/common/Arma Reforger/addons/data/data007.pak`:

- `scripts/Game/UI/Components/WidgetLibrary/Button/SCR_ButtonBaseComponent.c`: primary-click filtering and shared menu-select activation.
- `scripts/Game/Editor/Components/EditableEntity/SCR_EditablePlayerDelegateComponent.c`: GetControlledEntity resolves the editor proxy.
- `scripts/Game/Editor/Components/EditableEntity/SCR_EditableEntityComponent.c`: server transform path, scale preservation and editor session-load/navmesh callback.
- `scripts/Game/ScenarioFramework/Actions/SCR_ScenarioFrameworkActionSetEntityScale.c`: native SetScale application.
- `scripts/Game/GameMode/FactionManager/SCR_FactionManager.c`: relation setters and faction snapshot serialization.
- `scripts/Game/GameMode/Respawn/SCR_EditorRespawnBriefingComponent.c` and `SCR_RespawnBriefingComponent.c`: native inheritance, RewriteEntry_SA, reliable broadcast and RplSave/RplLoad.
- `scripts/Game/Map/ComponentsUI/SCR_MapJournalUI.c`: journal rebuild and selection-index behavior.
- `scripts/Game/Interactions/SCR_InteractionHandlerComponent.c`: native collection reset, override and nearby-owner lists.
- `scripts/Game/Editor/Components/Editor/SCR_PlayersManagerEditorComponent.c`: authoritative teleport and broadcast path.
- `scripts/Game/AI/Group/SCR_SuppressWaypoint.c`: suppression activity created when the waypoint becomes current.
- `scripts/Game/Entities/SCR_AIGroup.c`, `scripts/Game/AI/Components/SCR_AIGroupUtilityComponent.c`, `scripts/Game/generated/AI/AIGroupMovementComponent.c` and `scripts/Game/generated/AI/AIWaypoint.c`: waypoint/group and movement-handler contracts.
- `scripts/Game/Editor/Containers/Attributes/SCR_AIStanceEditorAttribute.c`: the stock editor attribute is unfinished, so its existence must not be mistaken for a working stance action; Bifrost uses the direct AI stance path.
- `scripts/Game/generated/InventorySystem/InventoryStorageSlot.c`, `BaseInventoryStorageComponent.c`, `WeaponAttachmentsStorageComponent.c`, and `scripts/GameCode/Components/InventorySystem/WeaponAttachmentsStorageComponent.c`: native storage/slot and inspection contracts used by Arsenal.
- `scripts/Game/Components/Fuel/SCR_FuelManagerComponent.c`: native fuel-manager servicing contract.
- `scripts/Game/Entities/Triggers/SCR_BaseTriggerEntity.c`: native activation/deactivation callbacks; Bifrost's richer conditions remain its server-owned logic.
- `Prefabs/Weapons/Ammo/Ammo_556x45_Tracer_M856.et`, `Ammo_556x45_Ball_M855.et` and `Prefabs/Weapons/Core/Ammo_Bullet_Base.et`: inherited damage effects and absence of an automatic cosmetic-mode switch.

From `data010.pak`:

- `UI/layouts/Editor/Attributes/AttributePrefabs/AttributePrefab_Checkbox.layout`: native bool attribute component and spinbox binding.
- `UI/layouts/WidgetLibrary/WLib_Slider.layout`: native slider root/component used by authored Arsenal controls.

The transient tracer draw flags were additionally checked against BI's [Shape and debug utility API](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/group__Debug.html): default depth comparison and default visibility are used; the cue is bounded by its retained lifetime.

The UI layouts are in data010, not data007; initial unsuccessful lookups were resolved against the actual archive. These inspections establish source contracts, not that a player completed the action in a running game.

## Validation and handoff

- `python Tests/verify_feature_layouts.py`: **358 binding checks, 25 layouts, zero failures**. Includes literal widget lookup checks, concrete widget types for mission/composition/CREATE and Arsenal screens, plus generated mission buttons and all eight composition rows. Two in-memory fault injections verified missing-control and wrong-type detection. Native ContextMenu/background and the verified Kits runtime render fallback are explicit exceptions.
- Local GUID-to-metadata scan: **205 layout/prefab/config references checked, zero mismatches**.
- Native WORKBENCH script validation: **zero errors, 14 existing native deprecation warnings**. Compilation includes all current source changes.
- Whitespace/diff review: passed.
- No live reload, automated game interaction, dedicated-server run, remote-client run or JIP run was performed. No Workshop/GitHub release was published.

Priority operator checks: non-primary clicks must not apply/delete/place; invincibility must affect a Player-tab selection and count selected crew once; repeated weapon and attachment clicks must update the draft and Wear must update equipment; Scale must be checked while walking/animating/driving and on a remote/JIP peer; a cosmetic tracer aimed at a character must cause no damage while remote clients hear/see it; LIVE must damage; deleting/moving overlapping terrain systems must restore only released scenery. Test all placement flows on clear ground first, then test invalid targets and cancellation.
