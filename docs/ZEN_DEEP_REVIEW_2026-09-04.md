# ZEN deep feature and portability review

Date: 2026-09-04

ZEN source baseline: [`zen-mod/ZEN` commit `b5786646a397593831232675a138cd6e35924356`](https://github.com/zen-mod/ZEN/tree/b5786646a397593831232675a138cd6e35924356), current `master` when the review began, described by the repository as v1.15.1 plus 22 commits.

Bifrost source baseline: the current `Bifrost-Dev` working tree. The tree already contained related unreleased composition, FX, visual, AI-order, and layout work; this review preserved it.

## Result

The ZEN repository contains 36 feature addons, 450 registered SQF functions, 64 documented modules, and 19 documented context-action groups. The review traced each addon from registration through execution and classified every public feature against Bifrost and the installed Arma Reforger 1.8 data.

Three missing outcomes met the complete portability gate and were implemented with their ZEN names:

1. **Suppressive Fire** — added to Bifrost Orders as Reforger's native replicated suppression waypoint.
2. **Place Comment** — added directly to the world context menu and routed into Bifrost's existing local/server comment editor.
3. **Teleport Players** — added for a selected set of players, with destination spacing, Game Master authorization, server execution through Reforger's native players manager, and terrain/player revalidation.

No ZEN source, textures, layouts, or assets were copied. ZEN is GPL-3.0; Bifrost is APL-SA. The implementations above reproduce user outcomes using original Enfusion code and shipped Reforger contracts.

## Portability gate

A missing ZEN outcome was marked **portable now** only when all of these were true:

- the installed Reforger build exposes a supported native contract for the outcome;
- the action can be server-authoritative and can reach remote clients reliably;
- it fits an existing Bifrost surface without introducing a second editor framework;
- it does not require ACE, TFAR, Advanced Rappelling, SQF, Arma 3 configuration namespaces, ZEN assets, or Arma 3-only content;
- the change can fail closed and has a bounded compile/static/runtime acceptance plan.

`Possible to recreate someday` is not treated as portable. A new cross-engine design needing new content, a new persistence model, unsafe arbitrary code execution, or an unproven multiplayer contract is classified as **not a direct port**.

## How ZEN executes

ZEN is a CBA-driven Arma 3 mod. Addon `XEH_PREP.hpp` files register functions; `XEH_preInit.sqf` compiles them and initializes settings/data; `XEH_postInit.sqf` installs CBA events and locality-specific handlers. `CfgVehicles.hpp` registers Zeus modules, while `CfgContext.hpp` declares context-menu conditions, statements, and dynamic children. UI-heavy features open display resources and attach display/map/3D event handlers. Gameplay mutations are commonly forwarded through CBA server/global events, public variables, or curator APIs.

The practical execution chain is:

`config/module/context registration -> prepared function -> local validation/UI -> CBA locality event -> server/global mutation -> curator/player presentation`

This is not source-portable to Reforger. Bifrost's corresponding chain is:

`native editor selection/action -> Bifrost UI -> reliable PlayerController RPC -> server rights and target revalidation -> native Enfusion mutation -> replicated state or reliable presentation RPC`

The distinction matters: ZEN often relies on Arma 3 global object state and CBA event routing, while Reforger requires explicit replication nodes, authority ownership, and join-in-progress handling.

## Addon-by-addon crosswalk

Status meanings: **Bifrost** is an authored Bifrost equivalent; **Native** is a shipped Reforger action Bifrost preserves; **Ported** was added in this review; **Partial** covers only part of the ZEN outcome; **Not direct** requires an Arma 3 dependency/content model or a new greenfield subsystem.

| ZEN addon | Registration and execution | User outcome | Bifrost/Reforger result | Status |
|---|---|---|---|---|
| `ai` | CBA/editor events call skill, garrison, grenade, and suppressive-fire helpers; authority-sensitive work is server-routed. | AI skill control, garrisoning, grenade throws, suppression. | Garrison/release already uses server-revalidated Bifrost world controls. `Suppressive Fire` now uses the shipped replicated suppress waypoint. Skill is available through native properties; forced grenade scripting has no stable generic GM contract. | Bifrost + Native + Ported |
| `area_markers` | Map display handlers create/edit/delete marker shapes and icons; events distribute changes. | Editable point and area markers. | Bifrost has local/server point, LZ, RP, target, intel, circle, rectangle, and comment records with paged editing and JIP snapshots. | Bifrost |
| `attached_objects` | Object-edited hooks and module calls attach/detach selected objects. | Attach and detach objects while preserving edits. | Bifrost Precise mode and the authoritative attach relay provide attach/detach. | Bifrost |
| `attributes` | Attribute registrations build controls, validate values, and write module/waypoint state. | Rich property dialogs for modules and waypoints. | Bifrost uses native Reforger editor attributes plus its own registered attribute list; writes to gameplay state are authority-owned. | Native + Bifrost |
| `building_markers` | Server creates grey rectangle map markers from Arma 3 building bounds and updates/deletes them on curator edits. | Make placed buildings resemble terrain buildings on the map. | Reforger's map and editor already render terrain and editable-entity representations. Recreating Arma 3 marker-brush semantics would require a new map renderer and does not improve the existing 3D bounds overlay. | Native; not direct |
| `camera` | Curator camera settings are updated from ZEN settings. | Adjustable camera speed/behavior. | Reforger ships manual-camera speed, acceleration, height, focus, terrain collision, save, and settings components; Bifrost keeps the native toolbar/input path. | Native |
| `comments` | Local and global comment events maintain icons, 2D/3D drawing, editing, deletion, and Eden persistence. | Place and manage curator comments. | Bifrost already had local/server comment records and JIP snapshots. This review added the exact `Place Comment` entry and changed the marker-kind label from `NOTE` to `COMMENT`. | Ported on Bifrost backend |
| `common` | Shared serialization, selection, firing, vehicle, UI, export, artillery, healing, and utility functions support other addons. | Internal support library, not one user-facing feature. | Mapped per consuming feature. SQF serialization/export and code execution are not portable; native Enfusion APIs replace other helpers. | Per consumer |
| `compat_ace` | Conditional ACE medical, captive, fast-rope, and remote-control bridges. | ACE interoperability. | ACE is not a Reforger dependency and its APIs do not exist. | Not direct |
| `compat_advanced_rappelling` | Detects Advanced Rappelling and exposes its action when available. | Third-party rappelling interoperability. | Dependency and SQF API do not exist in Reforger. | Not direct |
| `compositions` | Eden/curator displays edit the composition tree and create/delete/randomize entries. | Save and reuse object assemblies. | Bifrost has a persistent server-profile library with name/category/author, hierarchy-relative capture, terrain placement, atomic rollback, deletion, per-GM undo, and JIP catalog delivery. | Bifrost |
| `context_actions` | Conditions filter targets; statements invoke behavior, formation, healing, inventory, loadout, logistics, artillery, teleport, and other helpers. | Faster selected-entity operations. | Bifrost preserves evaluated native context actions and adds its own server-routed actions. `Teleport Players` was the remaining safe multi-player gap and is now ported. | Native + Bifrost + Ported |
| `context_menu` | Compiles config and script actions into the curator context tree. | Extensible ZEN action menu. | Bifrost rebuilds the visible menu from the currently evaluated native actions, preserves enabled state, and appends project actions. A second registry would duplicate the native editor. | Bifrost |
| `cover_map` | Four large local black map rectangles and a border mask everything outside a rotated mission area. | Restrict the visible operational map. | Reforger has no equivalent Arma 3 marker-brush mask contract. A correct version needs a separate map-render layer, clipping, resolution/input tests, and scenario persistence; that is a new feature, not a direct port. | Not direct |
| `custom_modules` | Local registry accepts scripted custom module callbacks and injects them into Zeus. | Runtime module extension framework. | Bifrost's editor placeables and native action/attribute registration are compile-time resources. Adding a parallel runtime module framework has no required user outcome and would increase conflict surface. | Native architecture |
| `damage` | Dialog reads named Arma 3 hit points and writes damage selections. | Configure object damage. | Native Reforger properties and damage manager actions expose health/damage; Bifrost Vehicle Service adds constrained repair diagnostics. | Native + Bifrost |
| `dialog` | Dynamic control factory creates checkbox/color/combo/edit/list/owner/side/slider/toolbox/vector dialogs. | Reusable module configuration dialogs. | Reforger editor attributes and Bifrost direct panels cover the UI outcome. Porting an SQF UI framework would add architecture, not a feature. | Native architecture |
| `doors` | Enumerates Arma 3 building animation sources, displays states, and writes them while tracking changes. | Configure selected doors. | Bifrost enumerates native door components and applies open/close to selected supported entities on the server. | Bifrost |
| `editor` | Curator display handlers add search, side/tree buttons, icons, pings, camera movement, stance shortcuts, and decluttering. | Modernized Zeus interface. | Bifrost replaces the operational shell while preserving native editor actions; it has searchable/paged CREATE and EDIT trees, native command import, top bar, context actions, and overlays. | Bifrost |
| `editor_previews` | Mouse handlers render curator asset previews. | Preview assets before placement. | Reforger's native asset metadata/previews are used by Bifrost's create catalog; the Arsenal and vehicle service use private render previews. | Native + Bifrost |
| `faction_filter` | Filters curator asset tree by faction. | Reduce asset-browser clutter. | Bifrost provides canonical APP-6 faction tabs, custom faction paging, search, and catalog filters. | Bifrost |
| `flashlight` | Toggles a light attached to the Arma 3 curator camera. | GM camera flashlight. | Reforger ships `SCR_LightManualCameraComponent`, `ManualCameraLight`, camera/pointing lights, intensity scaling, sound, and saved state. | Native |
| `garage` | Opens a 3D garage, populates animation/texture tabs, previews and applies vehicle appearance. | Configure vehicle appearance. | Bifrost safely covers vehicle preview, diagnostics, repair/refuel/rearm, authored service actions, and cargo. The installed API has no vehicle-agnostic livery/material/root-replacement contract that preserves identity, occupants, replication, and mod compatibility. | Partial; unsafe generic mutation excluded |
| `inventory` | Serializes cargo, opens an inventory editor, validates load, then applies changes. | Edit unit/vehicle inventory. | Bifrost Arsenal provides paged/searchable carried inventory and server-authoritative insert/remove/application. Vehicle Service covers cargo. | Bifrost |
| `loadout` | Captures default inventory, lists weapons, edits loadout, and confirms changes. | Edit/copy/paste/reset unit loadouts. | Bifrost Arsenal/Gunsmith/Kits provide edit, save, load, rename, overwrite, delete, reset, attachments, clothing, and cargo via authority. | Bifrost |
| `main` | Defines addon identity/version and shared macros. | ZEN package metadata. | Not a gameplay feature. | Not applicable |
| `markers_tree` | Compiles marker classes and manages category/tree/map selection and drawing. | Browse and place markers efficiently. | Bifrost has a paginated categorized marker tree and world/map records. | Bifrost |
| `modules` | Config classes invoke 92 prepared functions; CBA events separate server mutation from local/global presentation. | ZEN's 64 placed modules. | Every documented module is classified in the module matrix below. | Mixed |
| `music` | Extends the curator music tree with available Arma 3 music classes. | Browse/play mission music. | Arma 3 `CfgMusic` classes and assets do not exist in Reforger. Bifrost has positional ACP audio emitters, but a music browser would need an original catalog/content and playback-lifecycle design. | Not direct |
| `placement` | Curator tree/display hooks build and update placement previews. | Better pre-placement feedback. | Bifrost uses native placement and adds canvas-based bounds, height, action, trigger, zone, and range cues. | Bifrost |
| `position_logics` | Creates and cycles named helper logics for later module position selection. | Reuse named positions in module workflows. | Reforger uses world cursors, replicated entities, markers, and paired zones rather than SQF logic objects. | Native architecture; not direct |
| `pylons` | Reads compatible Arma 3 pylon magazines, mirrors selections, and applies loadouts/turret ownership. | Configure aircraft pylons. | Bifrost intentionally does not expose arbitrary armament or ammunition-type replacement. The public installed contracts are vehicle-authored and not generic across base/mod vehicles. Existing mounted ammunition may only be refilled. | Partial; unsafe generic mutation excluded |
| `remote_control` | Validates a unit and starts Zeus remote control, with compatibility bridges. | Control an AI unit. | Reforger ships `SCR_TakeControlContextAction`; Bifrost explicitly invokes and preserves it. | Native |
| `tasks` | Initializes task UI/state and updates task outcomes. | Create/manage tasks. | Reforger ships replicated task commands and task editor entities; Bifrost imports native Objectives commands and protects the native task-list layout. | Native |
| `visibility` | Per-frame player eye traces and smoke visibility draw red lines to visible cursor positions. | Show whether players can see a planned position. | Bifrost performs the visibility evaluation on the server for authorized GMs and returns viewer count/nearest distance; presentation uses the Bifrost render manager. | Bifrost |
| `vision` | Selects curator night/thermal modes and adjusts NVG brightness. | GM vision modes and brightness. | Reforger's manual camera and native editor toolbar own camera vision. Bifrost overlays AI vision/awareness but does not replace camera post-processing with Arma 3 thermal modes. | Native where available; Arma 3 modes not direct |

## Documented module matrix

The names below are the 64 module headings in ZEN's module guide. `Equivalent` means the operational outcome already exists; it does not imply copied implementation.

| ZEN module | Bifrost/Reforger mapping | Result |
|---|---|---|
| Add Full Arsenal | Bifrost Arsenal Access and unrestricted GM arsenal policy. | Equivalent |
| Ambient Animation | Bifrost Animations FX with native loiter/sit/lean/smoke/push-up/officer animations and server sessions. | Equivalent |
| Ambient Flyby | Bifrost Air Support flyby. | Equivalent |
| Artillery Fire Mission | Native Artillery Support command plus Bifrost live/cosmetic mortar emitter. | Equivalent |
| Atomic Bomb | ZEN-specific effect, damage, camera, audio, and Arma 3 asset chain. | Not direct |
| Attach/Detach Effect | Bifrost attach/detach plus independently placeable replicated emitters. | Equivalent workflow |
| Attach Flag | Place a flag/entity and use Bifrost authoritative attach. | Equivalent workflow |
| Attach To | Bifrost Precise attach/detach. | Equivalent |
| Bind Variable To Object | Depends on SQF mission namespaces and executable code strings. | Not direct |
| Change Height | Bifrost Precise vertical transform. | Equivalent |
| Change Weather | Bifrost Scenario weather controls/native manager. | Equivalent |
| Chatter | Arma 3 voice/event content and APIs have no direct Reforger contract. | Not direct |
| Configure Doors | Bifrost selected door open/close server action. | Equivalent |
| Convoy Parameters | Arma 3 convoy parameter model has no matching generic Reforger editor contract. | Not direct |
| Create Area Marker | Bifrost circle/rectangle editor. | Equivalent |
| Create/Edit Intel | Bifrost Intel marker and replicated Mission Info journal. | Equivalent |
| Create IED | Bifrost triggers, live explosive/rocket effects, and explosive placeables cover authored scenarios; transforming arbitrary objects into ZEN IEDs has no direct contract. | Equivalent workflow / no direct object conversion |
| Create LZ | Bifrost LZ marker. | Equivalent |
| Create Minefield | Reusable mine layouts can be saved as Bifrost compositions; mine entities remain native. | Equivalent workflow |
| Create RP | Bifrost RP marker. | Equivalent |
| Create Target | Bifrost target marker and native target/waypoint workflows. | Equivalent |
| Create Teleporter | ZEN uses Arma 3 object actions and SQF locality. Bifrost has GM/player teleports but no persistent public teleporter entity. | Not direct |
| Crew To Gunner | Native Reforger seat/crew context actions are preserved; no safe arbitrary seat reassignment override was added. | Native / partial |
| Custom Fire | Bifrost Tracer, Air Support gunrun, Loiter gunrun, and live projectile emitters. | Equivalent |
| Damage Buildings | Native damage properties and live ordnance; no generic percentage write across modded building damage graphs. | Native / partial |
| Earthquake | ZEN camera/environment effect is Arma 3-specific. | Not direct |
| Equip With ECM | Arma 3 aircraft sensor/countermeasure model has no equivalent Reforger API. | Not direct |
| Execute Code | Arbitrary SQF execution has no Enfusion equivalent and would be an unsafe remote-code surface. | Not portable |
| Export Mission SQF | SQF/Eden export is specific to Arma 3. | Not portable |
| Fly Height | Native Reforger waypoint/vehicle properties are preserved; Bifrost has flyby/loiter altitude controls. | Equivalent |
| Functions Viewer | Arma 3 developer viewer for SQF namespaces. | Not portable |
| Garrison Group | Bifrost server-revalidated garrison targeting and release. | Equivalent |
| Global Hint | Bifrost notifications/chat/tutorial surfaces; no need for an unrestricted global text module. | Equivalent workflow |
| Group Side | Bifrost Scenario side relations and native group properties. | Equivalent |
| Heal | Native `SCR_HealEntitiesContextAction` calls the damage manager on the server. | Native |
| Hide Zeus | Reforger role/editor visibility model differs; ordinary clients do not receive a Zeus logic object. | Not direct |
| Hide Terrain Objects | Bifrost nearby terrain hide/restore. | Equivalent |
| Light Source | Bifrost replicated ambient FX/audio plus native light placeables. | Equivalent workflow |
| Make Invincible | Bifrost server-routed Toggle Invulnerability. | Equivalent |
| Patrol Area | Native Reforger Patrol waypoint imported through Orders. | Native |
| Promote To Zeus | Reforger Game Master/server-role authorization is not an assignable Zeus logic object. | Not direct |
| Remove Arsenal | Bifrost Arsenal Access entities are editable/deletable and authorization is server checked. | Equivalent workflow |
| Rotate Object | Bifrost Precise rotation. | Equivalent |
| Set Date | Bifrost Scenario time/date. | Equivalent |
| Show In Animation Viewer | Arma 3 viewer is developer tooling; Bifrost Animations FX is the runtime authoring surface. | Not direct |
| Show In Config Viewer | Arma 3 config viewer is developer tooling. | Not portable |
| Side Relations | Bifrost Scenario side relations. | Equivalent |
| Sit On Chair | Bifrost Animations FX `Sit on chair`. | Equivalent |
| Smoke Pillar | Bifrost Heavy Smoke replicated ambient preset. | Equivalent |
| Spawn Reinforcements | Bifrost reinforcement task zone and native group placement. | Equivalent |
| Suicide Bomber | ZEN/Arma 3 AI and explosive behavior with no safe generic Reforger contract. | Not direct |
| Suppressive Fire | Native replicated `SCR_SuppressWaypoint`, exposed in Bifrost Orders under the exact ZEN name. | **Ported** |
| Teleport Players | Selected players are staged, spaced at the destination, server-authorized, and routed through the native players-manager teleport contract. | **Ported** |
| Toggle Flashlights | Native entity properties/actions where supported; Bifrost selected light controls cover world lights. | Native / equivalent |
| Toggle IR Lasers | Native weapon/property support is preserved; no unsafe inventory weapon mutation added. | Native where authored |
| Toggle Lamps | Bifrost selected light on/off server action. | Equivalent |
| Toggle Simulation | Bifrost Precise simulation control, server-routed with player exclusions. | Equivalent |
| Toggle Visibility | Bifrost visibility control and native editable visibility. | Equivalent |
| Tracers | Bifrost tracer emitter with six base-game calibers, live/cosmetic paths, cadence, bursts, sound, replicated state, and canvas diagnostics. | Equivalent |
| Un-Garrison Group | Bifrost garrison release restores AI state. | Equivalent |
| Update Editable Objects | Reforger auto-register/editable-entity systems and Bifrost native catalog merging. | Native |
| USS Freedom | Arma 3 carrier content is absent and cannot be redistributed. | Not portable |
| USS Liberty | Arma 3 destroyer content is absent and cannot be redistributed. | Not portable |
| Vehicle Turret Optics | Reforger's turret/optic ownership and camera model differs and exposes no generic GM optics replacement contract. | Not direct |

## Documented context-action matrix

| ZEN action | Execution outcome | Bifrost/Reforger result |
|---|---|---|
| Behavior | Writes selected AI group behavior. | Bifrost native/group order controls. |
| Captives | ACE or Arma 3 captive/surrender state. | Bifrost server-authoritative Surrender/Restore Selected AI; ACE-specific state is not portable. |
| Combat Mode | Writes group fire discipline. | Bifrost Fire at Will, Return Fire, Hold Fire. |
| Create Area Marker | Opens marker area creation. | Bifrost Markers & Intel circle/rectangle. |
| Door State | Opens/configures building doors. | Bifrost Open/Close Doors for Selection. |
| Editable Objects | Adds/removes curator editable objects. | Reforger auto-register/native editor plus Bifrost merged catalog. |
| Fire Artillery | Chooses artillery position and fires selected vehicle. | Native Artillery Support and Bifrost mortar workflow. |
| Formation | Sets group formation. | Bifrost Wedge, Vee/native, Line, Column, File/native, Staggered Column, Echelons/native, Diamond/native as available from current actions. |
| Heal | Heals selected crew by all/player/AI filter. | Native Reforger Heal action is preserved. |
| Inventory | Edit/copy/paste inventory. | Bifrost Arsenal inventory and cargo. |
| Loadout | Edit/copy/paste/reset/switch weapons. | Bifrost Arsenal/Gunsmith/Kits; native switching remains preserved. |
| Place Comment | Opens comment editor at cursor. | **Ported** direct entry on Bifrost's comment backend. |
| Remote Control | Possesses valid AI. | Native Take Control action is preserved and explicitly available in EDIT. |
| Speed | Sets group speed mode. | Bifrost Walk, Run, Sprint and native waypoint speeds. |
| Stance | Sets group stance. | Bifrost Stand, Crouch, Prone; native auto behavior resumes with normal AI. |
| Teleport Players | Selects a position/vehicle for selected players. | **Ported** for ground destinations; reliable native player teleport handles attached/vehicle state. |
| Teleport Zeus | Moves curator camera. | Native Reforger camera focus/teleport controls. |
| Vehicle Appearance | Opens garage or copies/pastes animation/textures. | Vehicle preview/service exists; unsafe arbitrary livery/material mutation remains excluded. |
| Vehicle Logistics | Repair, rearm, refuel, weapon selection, ViV unload. | Native resupply plus Bifrost Vehicle Service repair/refuel/refill-existing-mounted-ammo/cargo. Arbitrary weapon-type switching remains excluded. |

## Complete prepared-function inventory

This is the complete registration inventory at the pinned commit. Function names show the execution surface reviewed; the addon table above explains each addon's registration, locality, outcome, and Bifrost disposition.

- **ai (10):** `applySkills`, `canThrowGrenade`, `garrison`, `handleObjectEdited`, `handleSkillsChange`, `initMan`, `searchBuilding`, `suppressiveFire`, `throwGrenade`, `unGarrison`.
- **area_markers (20):** `applyProperties`, `configure`, `createIcon`, `createMarker`, `deleteIcon`, `isEditable`, `isInEditMode`, `onDraw`, `onKeyDown`, `onLoad`, `onMarkerCreated`, `onMarkerDeleted`, `onMarkerUpdated`, `onMouseButtonDown`, `onMouseButtonUp`, `onMouseDblClick`, `onMouseMoving`, `onUnload`, `onVisibilityPFH`, `updateIcon`.
- **attached_objects (4):** `attach`, `detach`, `handleObjectEdited`, `module`.
- **attributes (17):** `addAttribute`, `addButton`, `addDisplay`, `check`, `compileWaypoints`, `confirm`, `gui_checkboxes`, `gui_code`, `gui_combo`, `gui_edit`, `gui_icons`, `gui_loiter`, `gui_slider`, `gui_toolbox`, `gui_waypoint`, `handleMarkerPlaced`, `open`.
- **building_markers (3):** `handleObjectEdited`, `handleObjectPlaced`, `set`.
- **camera (1):** `updateSettings`.
- **comments (23):** `addDrawEventHandler`, `createComment`, `createIcon`, `deleteComment`, `deleteIcon`, `drawComments`, `is3DENComment`, `moveComment`, `onCommentCreated`, `onCommentDeleted`, `onCommentUpdated`, `onDraw`, `onDraw3D`, `onKeyDown`, `onMouseButtonDown`, `onMouseButtonUp`, `onMouseDblClick`, `onMouseMoving`, `onUnload`, `openDialog`, `save3DENComments`, `updateComment`, `updateIcon`.
- **common (75):** `canFire`, `changeGroupSide`, `collapseTree`, `createZeus`, `deployCountermeasures`, `deserializeInventory`, `deserializeObjects`, `displayCuratorLoad`, `displayCuratorUnload`, `drawHint`, `dumpPerformanceCounters`, `earthquake`, `ejectPassengers`, `exportMissionSQF`, `exportText`, `fireArtillery`, `fireVLS`, `fireWeapon`, `forceFire`, `forceWatch`, `formatDegrees`, `getActiveTree`, `getAllTurrets`, `getArtilleryETA`, `getCargoPositionsCount`, `getDefaultInventory`, `getDLC`, `getEffectiveGunner`, `getGunnerName`, `getLightingSelections`, `getPhoneticName`, `getPlayers`, `getPosFromScreen`, `getPylonTurret`, `getSelectedUnits`, `getSelectedVehicles`, `getSideIcon`, `getVehicleAmmo`, `getVehicleIcon`, `getWeaponReloadTime`, `hasDefaultInventory`, `hasPylons`, `healUnit`, `initDisplayPositioning`, `initListNBoxSorting`, `initOwnersControl`, `initSidesControl`, `initSliderEdit`, `isCursorOnMouseArea`, `isInScreenshotMode`, `isPlacementActive`, `isReloading`, `isRemoteControlled`, `isSwimming`, `isUnitFFV`, `isVLS`, `isWeapon`, `loadMagazineInstantly`, `messageBox`, `openArsenal`, `parseMagazineDetail`, `reloadDisplay`, `runAfterSettingsInit`, `selectPosition`, `serializeInventory`, `serializeObjects`, `setLampState`, `setMagazineAmmo`, `setTurretAmmo`, `setVehicleAmmo`, `setVehicleLaserState`, `showMessage`, `spawnLargeObject`, `teleportIntoVehicle`, `updateEditableObjects`.
- **compat_ace (5):** `canFastrope`, `canOpenMedicalMenu`, `canRemoteControl`, `openMedicalMenu`, `remoteControl`.
- **compat_advanced_rappelling (1):** `canRappel`.
- **compositions (12):** `buttonCreate`, `buttonDelete`, `buttonEdit`, `buttonRandomize`, `handleTreeChange`, `handleTreeSelect`, `initDisplay3DEN`, `initDisplayCurator`, `initHelper`, `openDisplay`, `processTreeAdditions`, `removeFromTree`.
- **context_actions (38):** `canEditInventory`, `canEditLoadout`, `canEditVehicleAppearance`, `canHealUnits`, `canPasteVehicleAppearance`, `canRearmVehicles`, `canRefuelVehicles`, `canRepairVehicles`, `canSwitchWeapon`, `canToggleSurrender`, `canUnloadViV`, `compileGrenades`, `copyVehicleAppearance`, `getArtilleryActions`, `getGrenadeActions`, `getVehicleWeaponActions`, `healUnits`, `openEditableObjectsDialog`, `pasteVehicleAppearance`, `rearmVehicles`, `refuelVehicles`, `repairVehicles`, `selectArtilleryPos`, `selectThrowPos`, `setBehaviour`, `setCombatMode`, `setFormation`, `setSpeedMode`, `setStance`, `switchVehicleWeapon`, `switchWeapon`, `switchWeaponModifier`, `teleportPlayers`, `teleportZeus`, `toggleCaptive`, `toggleSurrender`, `unloadViV`, `updateEditableObjects`.
- **context_menu (9):** `addAction`, `close`, `compileActions`, `createAction`, `createContextGroup`, `getActiveActions`, `initDisplayCurator`, `open`, `removeAction`.
- **cover_map (10):** `create`, `handleConfirm`, `handleDelete`, `handleDraw`, `handleLoad`, `handleMouseButtonDown`, `handleMouseButtonUp`, `handleMouseMoving`, `handleRotationChanged`, `remove`.
- **custom_modules (3):** `init`, `initDisplayCurator`, `register`.
- **damage (2):** `configure`, `getHitPointString`.
- **dialog (12):** `close`, `create`, `gui_checkbox`, `gui_color`, `gui_combo`, `gui_edit`, `gui_list`, `gui_owners`, `gui_sides`, `gui_slider`, `gui_toolbox`, `gui_vector`.
- **doors (7):** `configure`, `getActions`, `getDoors`, `getState`, `module`, `setState`, `updatePFH`.
- **editor (19):** `addGroupIcons`, `addModIcons`, `declutterEmptyTree`, `handleCuratorPinged`, `handleKeyDown`, `handleKeyUp`, `handleLoad`, `handleModeButtons`, `handleObjectPlaced`, `handleSearchButton`, `handleSearchClick`, `handleSearchKeyDown`, `handleSearchKeyUp`, `handleSideButtons`, `handleTreeButtons`, `handleUnload`, `moveCamToSelection`, `pingCurators`, `switchStance`.
- **editor_previews (3):** `handleMouseExit`, `handleMouseUpdate`, `initDisplayCurator`.
- **faction_filter (0 prepared functions):** config/event-only faction tree filtering.
- **flashlight (1):** `toggle`.
- **garage (17):** `applyToAll`, `closeGarage`, `getVehicleData`, `handleMouse`, `onAnimationSelect`, `onKeyDown`, `onMouseButtonClick`, `onMouseButtonDown`, `onMouseButtonUp`, `onMouseZChanged`, `onTabSelect`, `onTextureSelect`, `openGarage`, `populateLists`, `showVehicleInfo`, `toggleInterface`, `updateCamera`.
- **inventory (14):** `calculateLoad`, `clear`, `configure`, `confirm`, `getCargo`, `getItemMass`, `getWeaponItems`, `keyDown`, `modify`, `preload`, `refresh`, `reset`, `switchMode`, `update`.
- **loadout (9):** `clear`, `configure`, `confirm`, `fillList`, `getWeaponList`, `getWeaponName`, `keyDown`, `modify`, `updateButtons`.
- **main (1 macro placeholder):** `fncName`.
- **markers_tree (13):** `compile`, `handleAreasSelect`, `handleDraw`, `handleEngineSelect`, `handleIconsSelect`, `handleMouseButtonDown`, `handleMouseButtonUp`, `handleMouseMoving`, `handleSubModeClicked`, `handleTreeButtons`, `handleTreeChange`, `initDisplayCurator`, `populate`.
- **modules (92):** `addIntelAction`, `addTeleporterAction`, `bi_moduleCurator`, `bi_moduleMine`, `compileAircraft`, `compileCAS`, `compileEffects`, `compileFlags`, `compileMines`, `compileReinforcements`, `compileTracers`, `gui_ambientFlyby`, `gui_cas`, `gui_damageBuildings`, `gui_editableObjects`, `gui_executeCode`, `gui_fireMission`, `gui_globalHint`, `gui_setDate`, `gui_sideRelations`, `gui_spawnReinforcements`, `gui_tracers`, `initDisplay`, `initModule`, `moduleAddFullArsenal`, `moduleAmbientAnim`, `moduleAmbientAnimEnd`, `moduleAmbientAnimStart`, `moduleAmbientFlyby`, `moduleAnimationViewer`, `moduleArsenal`, `moduleAssignZeus`, `moduleAttachEffect`, `moduleAttachFlag`, `moduleBindVariable`, `moduleCAS`, `moduleChangeHeight`, `moduleChatter`, `moduleConvoyParameters`, `moduleCreateIED`, `moduleCreateIntel`, `moduleCreateLZ`, `moduleCreateMinefield`, `moduleCreateRP`, `moduleCreateTarget`, `moduleCreateTeleporter`, `moduleCreateTeleporterServer`, `moduleCrewToGunner`, `moduleDamageBuildings`, `moduleEarthquake`, `moduleEditableObjects`, `moduleEffectFire`, `moduleEffectFireLocal`, `moduleEquipWithECM`, `moduleExportMissionSQF`, `moduleFireMission`, `moduleFlyHeight`, `moduleFunctionsViewer`, `moduleGarrison`, `moduleGlobalAISkill`, `moduleGroupSide`, `moduleHeal`, `moduleHideTerrainObjects`, `moduleHideZeus`, `moduleLightSource`, `moduleMakeInvincible`, `moduleNuke`, `moduleNukeLocal`, `modulePatrolArea`, `moduleRemoveArsenal`, `moduleRotateObject`, `moduleScaleObject`, `moduleSearchBuilding`, `moduleShowInConfig`, `moduleSideRelations`, `moduleSimulation`, `moduleSitOnChair`, `moduleSmokePillar`, `moduleSpawnCarrier`, `moduleSpawnDestroyer`, `moduleSpawnReinforcements`, `moduleSuicideBomber`, `moduleSuppressiveFire`, `moduleTeleportPlayers`, `moduleToggleFlashlights`, `moduleToggleIRLasers`, `moduleToggleLamps`, `moduleTracers`, `moduleTurretOptics`, `moduleUnGarrison`, `moduleVisibility`, `moduleWeather`.
- **music (0 prepared functions):** config-driven curator music-tree extension.
- **placement (5):** `handleObjectPlaced`, `handleTreeChange`, `handleTreeSelect`, `setupPreview`, `updatePreview`.
- **position_logics (6):** `add`, `exists`, `get`, `initList`, `nextName`, `select`.
- **pylons (6):** `configure`, `handleConfirm`, `handleMagazineSelect`, `handleMirror`, `handlePreset`, `handleTurretButton`.
- **remote_control (4):** `canControl`, `handleMouseDblClick`, `module`, `start`.
- **tasks (2):** `init`, `update`.
- **visibility (3):** `draw`, `start`, `stop`.
- **vision (4):** `changeBrightness`, `setModes`, `showHint`, `updateEffect`.

## Implemented execution details

### Suppressive Fire

- User surface: `COMMAND -> WAYPOINTS -> Suppressive Fire`.
- Client: Bifrost resolves the shipped `PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Suppress_Editor.et` and enters native waypoint placement for all selected groups.
- Server/outcome: the replicated `SCR_SuppressWaypoint` creates a native suppression volume and `SCR_AISuppressActivity`; group AI select suitable weapons and sustain fire at the area until the timed waypoint ends, is replaced, or is cancelled.
- JIP: waypoint entity and group assignment use native replicated editor/AI systems.

### Place Comment

- User surface: empty-ground context menu `Place Comment`.
- Client: opens the Bifrost marker editor at the captured cursor position with `COMMENT` preselected.
- Local scope: record and presentation remain on the creating GM.
- Server scope: reliable mutation validates GM rights; snapshot serials deliver current comments to remote/rejoining GMs.
- Outcome: comment name, position, scope, world stick/label, tree entry, editing, and deletion all use the existing marker service.

### Teleport Players

- User surface: select one or more player entities, open context menu, choose `Teleport Players`, then right-click valid ground and choose `Teleport Players Here`.
- Client: captures unique player IDs, calculates a centered 1.25 m grid at the destination, and sends one reliable request per player.
- Server: rechecks Game Master rights, terrain bounds, player ID, live controlled entity, and players-manager availability; it invokes the shipped `TeleportPlayerToPositionServer` path.
- Presentation: the native players manager chooses owner or broadcast delivery according to attachment state and applies `SCR_Global.TeleportPlayer`, matching Reforger's own Game Master teleport behavior.
- Failure model: invalid/offline player IDs or out-of-bounds destinations are dropped without mutating another target; pending selection clears after dispatch.

## Validation

| Boundary | Result | Meaning |
|---|---|---|
| Workbench `ValidateScripts`, configuration `WORKBENCH` | Passed; 0 errors, 14 installed/base obsolete warnings | Current scripts compile. |
| Enfusion asset search | Found `PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Suppress_Editor.et` in installed game data | The exact suppress waypoint exists in the installed build. |
| PAC1CLI direct read | Confirmed editable suppress prefab -> `SCR_SuppressWaypoint` -> native suppression activity; confirmed native heal, player teleport, and camera light implementations | Classification and implementation use shipped contracts. |
| Layout identity scan | 178 object IDs and 71 widget names; 0 duplicate groups | The edited marker layout retained unique serialized identities. |
| `git diff --check` | Passed; line-ending notices only | No whitespace errors. |
| Dedicated server, remote client, JIP, hands-on UI | Not run in this review | Multiplayer design is present, but release readiness still requires the test matrix below. |

The 14 warnings are from installed base scripts and are unchanged obsolete-API warnings, not Bifrost compile errors.

## Required hands-on acceptance

1. **Suppressive Fire — listen server:** select one and several AI groups, place the waypoint, confirm sustained fire into the area, then replace/delete it and confirm fire stops.
2. **Suppressive Fire — dedicated/remote/JIP:** place as a remote GM; verify server-owned AI behavior for two observers and a late joiner.
3. **Place Comment — local:** create/edit/delete a local comment; verify a second GM never receives it.
4. **Place Comment — server/JIP:** create/edit/delete a server comment; verify remote and late-joining GMs receive the current state exactly once.
5. **Teleport Players — ground:** teleport one, two, and a full squad; verify centered spacing and no overlapping spawn pile.
6. **Teleport Players — vehicle/attachment:** teleport seated players and confirm the native teleport contract exits/repositions them consistently.
7. **Teleport Players — authority:** remote GM succeeds; ordinary client RPC attempts are refused; disconnect one selected player before placement and confirm other targets still complete.
8. **UI:** verify `COMMENT`, `Place Comment`, `Teleport Players`, `Teleport Players Here`, and `Suppressive Fire` at 1080p, ultrawide, keyboard/mouse, and controller.

## Primary sources

- [ZEN repository at the reviewed commit](https://github.com/zen-mod/ZEN/tree/b5786646a397593831232675a138cd6e35924356)
- [ZEN README feature summary](https://github.com/zen-mod/ZEN/blob/b5786646a397593831232675a138cd6e35924356/README.md)
- [ZEN GPL-3.0 license](https://github.com/zen-mod/ZEN/blob/b5786646a397593831232675a138cd6e35924356/LICENSE)
- [ZEN module guide](https://github.com/zen-mod/ZEN/blob/b5786646a397593831232675a138cd6e35924356/docs/user_guide/modules_list.md)
- [ZEN context-action guide](https://github.com/zen-mod/ZEN/blob/b5786646a397593831232675a138cd6e35924356/docs/user_guide/context_actions.md)
- [ZEN module registration](https://github.com/zen-mod/ZEN/blob/b5786646a397593831232675a138cd6e35924356/addons/modules/CfgVehicles.hpp)
- [ZEN context registration](https://github.com/zen-mod/ZEN/blob/b5786646a397593831232675a138cd6e35924356/addons/context_actions/CfgContext.hpp)
- [ZEN pre-init](https://github.com/zen-mod/ZEN/blob/b5786646a397593831232675a138cd6e35924356/addons/common/XEH_preInit.sqf)
- [ZEN post-init](https://github.com/zen-mod/ZEN/blob/b5786646a397593831232675a138cd6e35924356/addons/modules/XEH_postInit.sqf)

Installed Reforger evidence was read through Enfusion MCP, complete BI Wiki pages, and direct PAC1CLI access to `data007.pak`. Relevant shipped files were `SCR_HealEntitiesContextAction.c`, `SCR_TeleportPlayerHereContextAction.c`, `SCR_PlayersManagerEditorComponent.c`, `SCR_LightManualCameraComponent.c`, `E_AIWaypoint_Suppress_Editor.et`, `AIWaypoint_Suppress_Editor.et`, and `SCR_SuppressWaypoint.c`.

## Limitations

This is an exhaustive static feature/registration/execution review of the pinned ZEN source and current Bifrost tree, plus installed-API verification and Workbench compilation. It is not a claim that every Arma 3 runtime branch was executed, nor that the three new actions have passed a dedicated-server, remote-client, controller, or JIP play session. Those remain explicit release gates above. The host policy refused recursive removal of the validated temporary research clone, so `C:\Users\Bryce\AppData\Local\Temp\codex-zen-review-20260904` remains on disk; no process from it remains running.
