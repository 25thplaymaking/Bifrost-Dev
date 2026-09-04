# Bifrost GM parity implementation and release test plan

Date: 2026-08-31

Project: Bifrost-Dev

Branch: `release/v1.0.28`

Scope: the six-phase native Reforger Game Master and Arma 3 Zeus/ZEN/Achilles parity roadmap, with the vehicle Arsenal service zone implemented last.

## Outcome

All six planned implementation phases are wired into the current working tree. The final phase adds an editor-placeable Bifrost Vehicle Service Bay. A player parks inside its 9 m service circle, stops, exits, and uses a separately movable access marker. The GRS-styled workshop screen previews the selected vehicle, reports native hit-zone damage and mounted ammunition, installs vehicle-authored pylon choices, runs timed repair/refuel/rearm/full-service operations, and manages cargo from the active Arsenal catalog.

For a short one-feature-at-a-time test sequence, use docs/GM_PARITY_QUICK_TEST_GUIDE_2026-08-31.md.

There are no dormant buttons or placeholder handlers in the new panels. Unsupported generic vehicle transformations are omitted rather than presented as controls that cannot safely complete. In particular, arbitrary prefab replacement, livery/material mutation, and unknown modded attachment-slot mutation are not attempted because the installed API does not expose one vehicle-agnostic contract that preserves identity, occupants, replication, damage, and persistence. Supported native subsystems work independently and unsupported ones return an explicit result without mutating the vehicle.

## Decision record

```text
REQUIREMENTS
- Complete the full GM parity roadmap; implement the vehicle phase last.
- Make batch actions, markers/intel/visibility, AI/world controls, compositions,
  specialist orders/reinforcements/effects, and vehicle service usable end to end.
- Keep gameplay mutations authoritative on the dedicated server.
- Back implementation decisions with Enfusion MCP, complete BI Wiki reads, and
  direct PAC1CLI inspection of regular game data.pak archives.
- Reuse Bifrost/GRS visual language and provide an actionable test matrix.
- Use no more than five subagents and review all delegated work in the main task.

MINIMUM COMPONENTS NEEDED
- Shared batch target/authority/result helpers.
- Marker tree, area editor, server/local marker model, and visibility indicator.
- One authoritative AI/world-control service and existing GM action surfaces.
- A bounded server-session composition library and compact placement panel.
- Native specialist waypoints, reinforcement zone, and replicated ambient FX.
- One replicated service-zone prefab plus a constrained replicated access marker,
  one authority/RPC service, one private workshop preview, and one GRS-themed dialog.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No external Arma 3 source was copied.
- No arbitrary vehicle prefab, livery, material, or unknown slot replacement.
- No dedicated-server, remote-client, or JIP success claim without that run.

PRIMARY RISKS
- Modded vehicles expose different damage, fuel, weapon-rack, and storage graphs.
- Inventory and editor actions can look successful locally while failing authority.
- Generic recursive entity traversal can accidentally treat cargo weapons as mounts.
- The broad pre-existing dirty worktree must be preserved.

REQUEST INTERPRETATION
- Vehicle "loadout" means safe mounted ammunition service, vehicle-authored pylon choices, and editable cargo.
- The service bay applies only native contracts discovered in the installed build
  and fails closed when a vehicle does not expose one of them.
```

## Required evidence chain

The project rule was followed in order for each subsystem:

1. **Enfusion MCP** — API and component searches, native resource inspection, Workbench resource resolution, script validation, and GM_Arland smoke testing.
2. **BI Wiki through MCP** — complete local pages were read for Game Master, context actions, replication/multiplayer scripting, composition configuration, editable entities, car modding, damage, weapon components, and related editor systems.
3. **PAC1CLI** — `C:\Users\Bryce\pac1-cli\pac1cli.exe` directly listed, read, and extracted relevant files from `P:\SteamLibrary\steamapps\common\Arma Reforger\addons\data\data*.pak`. PAC1CLI was available and used successfully.

Representative direct `data007.pak` receipts include:

- `scripts/Game/Editor/Components/EditableEntity/SCR_EditableEntityComponent.c`
- `scripts/Game/Editor/Containers/Actions/ContextActions/SCR_PlaceEntityContextAction.c`
- `scripts/Game/Editor/Containers/Actions/ContextActions/SCR_RefillMagazineContextAction.c`
- `scripts/Game/Editor/Containers/Actions/ContextActions/SCR_BaseRocketPodsContextAction.c`
- `scripts/Game/Editor/Containers/Actions/ContextActions/SCR_RearmRocketPodsContextAction.c`
- `scripts/Game/Components/SupportStation/SCR_BaseSupportStationComponent.c`
- `scripts/Game/Components/SupportStation/SCR_FuelSupportStationComponent.c`
- `scripts/Game/Components/SupportStation/SCR_RepairSupportStationComponent.c`
- `scripts/Game/Components/SupportStation/SCR_VehicleWeaponSupportStationComponent.c`
- `scripts/Game/Components/Fuel/SCR_FuelManagerComponent.c`
- `scripts/Game/Components/Damage/SCR_VehicleDamageManagerComponent.c`
- `scripts/Game/Inventory/Callbacks/SCR_PrefabSpawnCallback.c`
- `scripts/Game/generated/InventorySystem/InventoryStorageManagerComponent.c`
- `scripts/Game/generated/InventorySystem/InventoryTask/ScriptedInventoryOperationCallback.c`

The full PAC extraction used for the final vehicle review contained 5,809 script files and completed with zero extraction errors. It lives outside the project and is not part of the release artifacts.

## Implemented feature matrix

| Phase | Completed behavior | Authority and failure model |
|---|---|---|
| 1. Batch and multi-selection foundation | Normalizes selected editable entities, resolves category-safe target sets, centralizes GM authorization, and aggregates per-target results. | Server-facing consumers re-resolve target IDs and reject invalid/player targets as appropriate. |
| 2. Marker, intel, and visibility tools | Paginated marker tree; local and server markers; point, area, objective, warning, route and intel kinds; edit/delete; size/rotation controls; placement cursor; configurable player-visibility awareness. | Server marker mutations require GM rights; local markers remain local; snapshots are serial-gated for refresh/JIP ordering. |
| 3. AI and world controls | Doors open/close, lights on/off, surrender/release, garrison/ungarrison, existing combat/speed/stance controls, and state restoration. | Reliable server RPCs, GM-rights checks, player exclusion, server-owned state records, and native AI/door/light contracts. |
| 4. Persistent compositions | Capture selected editable entities, store relative transforms/hierarchy/terrain offsets in a versioned server-profile library, place atomically, delete catalog entries, and undo the last placement per GM. | Server re-resolves every selected RplId and resource; invalid placement aborts before spawn; partial failure rolls back all spawned entities; failed library writes roll back the capture/delete. |
| 5. Specialist orders, reinforcements, and effects | Native Scout, Wait, Load, and Unload waypoint access; reinforcement task zone; replicated ambient Campfire, Heavy Smoke, Electric Sparks, and Fireflies presets. | Native waypoint placement owns command semantics; zone/effect state is authoritative and replicated; dedicated servers skip visual rendering. |
| 6. Vehicle Arsenal service bay | Placeable 9 m bay; movable in-circle access marker; exit-vehicle gate; private workshop preview with orbit/zoom/reset; damage and mounted-ammo diagnostics; native authored mount choices; timed repair, refuel, rearm, pylon installation, and full service; searchable/paginated cargo with fit feedback. | The client never mutates the gameplay vehicle. A reliable request starts the server-owned timer; cancellation is reliable; completion revalidates caller, access point, zone, vehicle, distance, stopped state, destroyed state, catalog membership, fit, authored action/slot/prefab compatibility, and native operation results before mutation. |

Adjacent roadmap work in the same release tree also includes native context-action preservation and pagination, native editor toolbar access, APP-6/create-panel corrections, selected-group action/path cues, rotated debug bounds, completion-radius replication and white-circle regeneration, Arsenal text/layout fixes, clothing/gear cargo editing, and gunsmith transition guards. These remain part of the broad release diff and were preserved during this pass.

## Phase 6: vehicle Arsenal design

### User flow

1. A GM places **Bifrost Vehicle Service Bay** from the editor catalog.
2. A player drives or tows a vehicle into its visible white 9 m circle.
3. The player stops and exits, then approaches the separately movable **Vehicle Service Access Point**.
4. The player uses **Open Vehicle Service**; no action is offered while seated or away from that marker.
5. The dialog lists every replicated root vehicle in range, marks it `STOPPED` or `MOVING`, and renders the selected prefab in a private workshop world.
6. The player inspects damage or mounted ammunition, installs a pylon option from **Mounts**, manages cargo, or starts repair, refuel, rearm, or full service.
7. Timed service cancels if the vehicle moves, the player enters it, the player leaves the access point, or the vehicle leaves the circle.
8. Starting service sends one reliable request. The server validates it, owns the required duration, and permits only one pending timed service per controller. The client mirrors progress and sends a reliable cancellation when the interaction becomes invalid. At expiry, the server revalidates everything, applies the change, and returns a correlated result to that menu instance.

The menu refreshes vehicle, native hit-zone health, mounted ammo, authored pylon options, and cargo state every 500 ms. It pages every bounded list, filters the active Arsenal catalog, shows `FITS`/`DOES NOT FIT`, and closes if the user leaves the access point. The preview is local-only and physics-disabled; the real replicated vehicle remains in the gameplay world.

### Server validation

Every mutation requires all of the following on the server:

- a valid reliable request verb and bounded payload;
- for repair/refuel/rearm/armament/full service, one pending request per controller and the server-owned 25/20/15/6.5/60-second duration;
- a currently replicated service-zone RplId;
- a currently replicated vehicle RplId normalized to a `BaseVehicle` root;
- the requesting controller's current controlled entity;
- the user outside any vehicle, within 4.5 m of the service access marker, and still inside the owning service zone;
- the selected vehicle in the same world and inside the 3D 9 m service radius;
- valid vehicle physics with linear speed at most 1 m/s and angular speed at most 0.15 rad/s;
- a non-destroyed native damage state when a damage manager exists;
- active Arsenal-catalog membership for cargo additions;
- a server-side `CanInsertResource` result for deposit storage;
- a successful native inventory insertion or removal operation.
- for pylon installation, an `SCR_AttachPylonSupportStationAction` authored on the selected vehicle whose linked `WeaponSlotComponent`, item prefab, and stable hierarchy key all match the request.

The owner response carries a monotonically increasing request ID. Closing, changing vehicles, entering the vehicle, leaving the marker, moving the vehicle, or leaving the circle cancels the matching server timer. A delayed response from a closed dialog cannot update a newly opened dialog.

### Mounted weapon safety correction

The first implementation recursively visited all vehicle children. The adversarial review identified that inventory cargo can appear in that child tree, which could refill carried weapons. The final implementation shares one mounted-slot discovery path between the Ammo UI and the server mutation:

- `WeaponSlotComponent` instances on the vehicle root and its non-inventory authored hierarchy;
- direct or slot-authored turret holders reached through that hierarchy;
- every inventory-item child branch excluded unless that branch is itself a native `Turret`;
- their muzzle magazines and native rocket ejector barrels.

Cargo weapons are never part of the rearm traversal. Already-full supported systems return success without changing state. Vehicles with no supported mounted ammunition return a clear unsupported result.

### Supported and intentionally unsupported vehicle capability

Supported independently:

- native vehicle damage repair, excluding destroyed vehicles;
- every native fuel manager on a composite vehicle;
- current mounted turret magazines, refilled on the authority;
- authored rocket-rack barrels;
- pylon choices exposed through the vehicle's native `SCR_AttachPylonSupportStationAction` definitions;
- deposit-purpose vehicle cargo from the active Arsenal catalog.

Intentionally absent:

- replacing the root vehicle prefab;
- arbitrary livery or material swapping;
- unknown modded attachment slots without a native attach-action contract;
- arbitrary turret-magazine type substitution: the public magazine API supports authority-side refill, while native `DoReloadWeaponWith` requires a compatible entity already owned by the correct turret inventory and controller;
- resurrecting destroyed vehicles;
- filling cargo-carried weapons as if they were mounted.

This is a completed safety boundary, not a half-wired future button.

## Main-agent review stamp

Four unique bounded subagent roles were used, below the maximum of five. Existing roles were reused for follow-up review instead of spawning additional agents.

1. **Batch foundation — accepted.** Main review confirmed the helpers do not mutate gameplay state by themselves and consumers retain server authority.
2. **Native/Zeus parity research — accepted.** Primary feature findings were mapped to installed Reforger contracts; no external source was copied.
3. **Marker/intel and runtime-composition implementation — accepted.** Main review verified server rights, RplId re-resolution, serial-gated snapshots, hierarchy capture, atomic rollback, resource/layout registration, and compile results.
4. **AI/world implementation and Phase 6 adversarial review — accepted with corrections.** Main review applied the mounted-only shared discovery path, full 3D range checks, exit/access-point gate, fail-closed motion checks, private preview cleanup, root-vehicle/damage lookup, server-timed service/cancellation, authoritative cargo results, request correlation, and truthful service-circle copy.

The main agent independently reviewed the authority path and ran resource/GUID/widget/comment/whitespace checks. The last focus-free Workbench validation after the primary composition and vehicle-workshop integration passed. The final mounted-only consolidation and documentation pass was made after the injected MCP handlers had already been removed while the user's Workbench session remained active, so the current-runtime compile and hands-on rows below remain explicit test boundaries rather than being silently inferred.

## Automated validation results

| Boundary | Result | What it proves |
|---|---|---|
| `wb_validate_scripts`, configuration `WORKBENCH` | Last integration pass: 0 errors, 14 installed/base-game obsolete warnings | Primary composition and vehicle-workshop implementation compiled. Re-run after the final mounted-only consolidation when the active test session is available. |
| Workbench resource DB: ambient FX prefab | Status 0, correct GUID and component graph | Registered and resolvable. |
| Workbench resource DB: original vehicle service prefab | Status 0, correct GUID and 9 m service-zone graph before the separate access-marker retrofit | Baseline service resource resolved; the new access and workshop resources require the current hands-on/resource pass. |
| Workbench resource DB: dialog/placeable configs | Status 0; vehicle dialog and service prefab present | UI preset and GM placement catalog are wired. |
| Layout resource lookup | Marker/composition identities resolved in the earlier integration pass; the cleaned-handler state prevented a new vehicle-layout `getInfo` call without taking over the user's session | Static layout checks are recorded separately; runtime parsing is not overstated. |
| Enfusion API and PAC1CLI mounted-weapon audit | Native source confirms authority-side current-magazine refill, inventory-owned magazine reload, and authored pylon replacement through `TurretControllerComponent`; support-station ACP repair/refuel events and workshop resources were resolved from `data.pak` | The generic refill and authored-mount paths follow native contracts; arbitrary unknown ammunition injection is deliberately absent. |
| New-layout object IDs/names | Static scan: zero duplicate object IDs or widget names in the new composition and vehicle layouts | No intra-layout identity collision. |
| Workspace `.meta` GUID scan | 400 resources, zero duplicate GUID groups | No resource GUID collision. |
| Comment/TODO/process sweep | No TODO, FIXME, tool-history, prompt, workflow, or temporary-process comments in new subsystem files | Project commenting rule is satisfied. |
| `git diff --check` | Pass; only Windows LF-to-CRLF notices | No whitespace error. |
| Earlier GM_Arland baseline smoke | Original service bay and armed M1025 instantiated together | The pre-retrofit bay and representative vehicle coexisted; it does not prove the new private preview/access-marker flow. |
| Current workshop runtime, dedicated remote, and JIP | Pending the itemized user pass below | No runtime, multiplayer, or JIP success is inferred from static or earlier baseline evidence. |
| Temporary smoke state | Both test entities deleted without saving; no separate Reforger game process remained | The base world was not modified by the smoke setup. |

The 14 compile warnings are unchanged installed base-game obsolete API warnings. Existing duplicate ban/kick command registration and native notification-key warnings are outside this patch and are not presented as Bifrost regressions.

The final bay intentionally retains the native white `SCR_SupportStationAreaMeshComponent` but removes the prototype's fuel, repair, and weapon support-provider components. This prevents ordinary proximity support actions from bypassing the exit/access-point gate and timed workshop controls; native ACP events are reused strictly as audiovisual feedback.

## Release testables

### A. Batch and multi-selection

- **A1 — mixed selection normalization:** select units, groups, vehicles, and props; batch-capable actions act only on their supported categories and report skipped targets.
- **A2 — player safety:** include a player in a destructive/freeze/surrender selection; the player is rejected while valid AI targets complete.
- **A3 — remote GM authority:** execute a batch action as a remote GM and confirm server state matches every successful row.
- **A4 — ordinary client denial:** a non-GM cannot invoke the server mutation RPCs.

### B. Markers, areas, intel, and visibility

- **B1 — local marker isolation:** create/edit/delete each marker kind as local; a second client never receives it.
- **B2 — server marker replication:** create point and area server markers; remote and late-joining GMs receive name, kind, position, size, and rotation.
- **B3 — tree pagination:** exceed one page, edit entries on both pages, and verify selection remains correct after refresh.
- **B4 — visibility indicator:** toggle indicator, placement-only/always modes, and range; verify cues appear only under their configured conditions.
- **B5 — permission:** ordinary clients cannot mutate server marker records.

### C. AI and world controls

- **C1 — doors/lights:** apply open/close and on/off to selected supported entities; unsupported targets report cleanly.
- **C2 — surrender/release:** surrender AI groups, then release them; weapons/AI state restore without affecting player characters.
- **C3 — garrison/ungarrison:** garrison a group into a valid building, then restore it; remote clients observe the same positions/state.
- **C4 — group behavior:** test Fire at Will, Return Fire, Hold Fire, Walk, Run, Sprint, stance, and manual hold/release.
- **C5 — dedicated/JIP:** change each persistent world-control state, join late, and confirm native/replicated state agrees.

### D. Runtime compositions

- **D1 — simple capture/place:** capture several props and place them at a new terrain cursor position.
- **D2 — hierarchy:** capture a parent plus children; verify relative transforms, scale, and parent relationships.
- **D3 — missing external parent:** capture a child without its editor parent; it becomes a valid composition root.
- **D4 — uneven terrain:** place multi-root composition over uneven ground and confirm root alignment preserves descendant offsets.
- **D5 — atomic rejection:** remove/block one captured resource and confirm placement spawns nothing.
- **D6 — rollback:** force a hierarchy-link/spawn failure and confirm all partial entities are deleted.
- **D7 — undo ownership:** two GMs place compositions; each can undo only their own latest placement.
- **D8 — remote/JIP catalog:** open as a remote GM and late join; receive the current persistent server catalog once, without duplicate/stale rows.
- **D9 — persistence:** capture a composition, restart the world/server, and confirm the same catalog entry can be placed afterward.

### E. Specialist orders, reinforcement, and ambient effects

- **E1 — waypoints:** place Scout, Wait, Load, and Unload on selected groups and confirm native completion behavior.
- **E2 — completion radius:** change radius; AI arrival behavior and the white order circle change together on server, remote client, and JIP.
- **E3 — reinforcement zone:** place/configure the zone and confirm eligible reinforcement behavior completes authoritatively.
- **E4 — ambient presets:** place each ambient FX preset; clients render it, dedicated server owns state without rendering visuals.
- **E5 — effect lifecycle:** change/disable/delete an emitter and confirm remote clients remove the prior native effect.

### F. Vehicle Arsenal service bay

- **F1 — GM placement/access:** place two bays; each renders a 9 m white circle and one separate movable access marker. Move each marker and verify it clamps inside its own circle and survives JIP.
- **F2 — exit and access range:** while seated, confirm no action; stop, exit, approach within 4.5 m, open the menu, then walk away and confirm it closes. A vehicle origin outside 9 m is excluded.
- **F3 — private workshop:** select multiple base/mod vehicles; drag to rotate, zoom, and reset. Verify the correct model is centered above the floor and the gun table/wall/shelves are absent.
- **F4 — multiple vehicles:** park more than five vehicles, page the list, select each, and confirm cargo/service applies only to the selected root vehicle.
- **F5 — timed cancellation:** begin each operation, then separately move/rotate the vehicle, enter it, walk away, or leave the circle. Progress cancels and no mutation occurs.
- **F6 — motion rejection:** test linear movement, in-place rotation, and a modded vehicle with no resolved physics; every operation is rejected.
- **F7 — repair/damage:** damage several native hit zones; verify lowest-health systems sort first and percentages update, then repair. A destroyed vehicle remains destroyed and is explicitly rejected.
- **F8 — multi-tank fuel/audio:** drain a vehicle with multiple fuel managers; verify timed progress and native refuel audio, then confirm all reach full only after the server response.
- **F9 — turret magazine rearm:** partially empty an authored mounted belt/magazine and confirm the Ammo tab reports it and timed Rearm fills it.
- **F10 — rocket rack rearm:** empty authored rocket barrels and confirm only reloadable barrels receive default rockets.
- **F11 — cargo isolation:** put a partially empty weapon in vehicle cargo, run Rearm, and confirm it is absent from Ammo and remains unchanged.
- **F12 — already full/unsupported:** full supported ammo reports success; a vehicle with no mounted supported ammo reports unsupported without mutation.
- **F13 — cargo fit:** fill cargo near capacity; `FITS`/`DOES NOT FIT` and the server result agree before and after each add.
- **F14 — cargo authority:** add/remove multiple items; counts and result notifications reflect successful server inventory operations, not optimistic client changes.
- **F15 — authored pylon choices:** use a base helicopter with pylon actions, then a modded vehicle that authors native attach actions. **Mounts** shows only those action/slot/prefab pairs, identifies the installed choice, and changes a different choice only after 6.5 seconds. A fabricated or stale key is rejected.
- **F16 — stale response:** send a request, close/reopen the dialog, and confirm the old response cannot update the new dialog.
- **F17 — catalog policy:** an RPC payload not in the active Arsenal catalog is rejected server-side.
- **F18 — dedicated remote:** perform every mutation from a remote player on a dedicated server; server and all observers agree.
- **F19 — JIP:** move the access marker and service damage/fuel/ammo/mount/cargo, then join late; the new client receives the native replicated state.
- **F20 — unsupported mod vehicle:** test a vehicle exposing only some systems; supported operations work independently and unsupported ones return a clear message.

### G. Release UI regression sweep

- **G1 — panel exclusivity:** marker, composition, context, and vehicle dialogs do not stack or strand input contexts.
- **G2 — resolution/input:** test 1080p, ultrawide, controller, and keyboard/mouse; no text overflow and correct back/close prompts.
- **G3 — APP-6/create panel:** verify unsquished create icons and placed-squad icon changes as composition/size changes.
- **G4 — Arsenal/Gunsmith:** retest carousel, gear cargo, compatible magazines, item attachments, Soldier-tab save confirmation, and AK/table clearance.
- **G5 — dedicated debug visuals:** verify paths, order/action glyphs, placement-height cues, and rotated bounds use the replicated/render-manager path rather than local Workbench-only debug drawing.

## Validation boundary and release recommendation

The current tree is ready for the listed hands-on pass once Workbench performs its normal script/resource refresh. Static structure, installed API contracts, and the last integration compile pass are clean; the final mounted-only consolidation was intentionally not followed by an MCP-driven Workbench takeover while the user was testing. Current private-workshop runtime, dedicated-server authority, remote-client observation, controller navigation, and JIP are explicitly unverified until their corresponding testables run.

The cleanup gate confirms `Scripts/WorkbenchGame/EnfusionMCP` is absent from Bifrost-Dev, so no injected MCP handler ships with the package.

## Primary implementation files

- `Scripts/Game/DCO/GMUI/Batch/`
- `Scripts/Game/DCO/GMUI/Markers/`
- `Scripts/Game/DCO/GMUI/World/`
- `Scripts/Game/DCO/GMUI/Compositions/`
- `Scripts/Game/DCO/GMUI/Orders/DCO_GMCommandOrders.c`
- `Scripts/Game/DCO/Effects/DCO_FxAmbient.c`
- `Scripts/Game/DCO/Arsenal/Vehicle/DCO_VehicleServiceZone.c`
- `Scripts/Game/DCO/Arsenal/Vehicle/DCO_VehicleServiceMenu.c`
- `Scripts/Game/DCO/Arsenal/Vehicle/DCO_VehiclePreviewStage.c`
- `Prefabs/E_DCO_FxAmbient.et`
- `Prefabs/E_DCO_VehicleServiceZone.et`
- `Prefabs/E_DCO_VehicleServiceAccess.et`
- `Prefabs/UI/GRSA_VehicleStageEnvironment.et`
- `UI/layouts/DCO_GMMarkers.layout`
- `UI/layouts/DCO_GMCompositions.layout`
- `UI/layouts/Menus/ArmoryV2/GRSA_Shell.layout` and `GRSA_ScreenGunsmith.layout` (reused directly by Vehicle Service)
- `Configs/Dialogs/DCO_ArsenalDialogs.conf`
- `Configs/Editor/PlaceableEntities/DCO_PlaceableEntities.conf`

The working tree contained substantial related user work before this final roadmap pass. No unrelated changes were reset, reformatted, or removed.

## Arsenal Access create-row icon decision — 2026-09-02

REQUIREMENTS
- Show an appropriate icon beside the virtual Arsenal Access entry in the GM Create panel.
- Preserve the entry's existing location and object-targeting behavior.

MINIMUM COMPONENTS NEEDED
- Assign the existing GRS Arsenal crate texture to the synthetic catalog entry.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No new icon asset, layout, prefab, or catalog abstraction is needed.

PRIMARY RISKS
- An invalid resource reference would leave the row blank; use the already registered texture used by the Arsenal UI.

REQUEST INTERPRETATION
- The missing glyph in the supplied Create-panel screenshot is the only requested behavior change.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Give Arsenal Access a clear Arsenal-specific icon without changing how the GM places or uses it.

## Vehicle Service row cleanup decision — 2026-09-02

REQUIREMENTS
- Remove text overlap and inconsistent wrapping across Service, Armaments, and Cargo.
- Preserve the GRS Arsenal shell, item row, controls, and current behavior.
- Correct other directly evidenced spacing/readability defects in the same UI sweep.

MINIMUM COMPONENTS NEEDED
- Reuse the existing row's name, state, and metadata columns instead of composing every value into the state field.
- Apply Service Bay-only row sizing, rail width, and background opacity adjustments.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No replacement layout, new design system, or unrelated Service Bay feature work is needed.

PRIMARY RISKS
- Editing the shared Arsenal row layout could regress player Arsenal screens, so Service Bay adjustments remain runtime-local except for the already Service-only opaque-row helper.

REQUEST INTERPRETATION
- Clean up every currently active Service Bay tab, with the supplied Service list as the primary visual defect and prior Armaments/Cargo evidence included in the sweep.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Make the current GRS-derived Service Bay interface legible and consistent without changing its interaction model.

## 2026-09-02 service release-scope update

- Armaments is removed from the release scope. The active mode strip contains only Repair and Cargo.
- Repair retains server-authoritative Repair, Refuel, Rearm, and Full Service; Rearm only refills already-mounted authored magazines and rocket barrels.
- Earlier Armaments implementation and testing notes in this audit are historical and superseded.
