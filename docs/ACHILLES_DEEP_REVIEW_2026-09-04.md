# Achilles deep feature and portability review

Date: 2026-09-04

Achilles source baseline: [`ArmaAchilles/Achilles` commit `f123656459cab7766aa40c32d5ee12d29ebadaae`](https://github.com/ArmaAchilles/Achilles/tree/f123656459cab7766aa40c32d5ee12d29ebadaae), the repository's current `master` when this review began. The repository identifies the last packaged tag as `v1.3.1` and states that core development moved to ZEN.

Bifrost source baseline: the current `Bifrost-Dev` working tree, including the preceding ZEN portability batch and the user's unreleased composition, loiter, visual, layout, and replication changes. No existing work was reset or replaced.

## Result

The inspected tree contains 662 non-Git files, nine addon packages, 294 `fn_*.sqf` implementation files, 100 non-custom module/config entries, 50 user-defined module slots, five scripted waypoint types, ten CBA keybinds, and 66 packaged Ares compositions beneath 15 composition subcategories.

One missing Achilles outcome passed the complete portability gate and was implemented under its exact Achilles name:

- **Change Side Relations** — a three-stage GM picker chooses the first side, second side, and Friendly/Hostile result. The request uses Bifrost's reliable player-controller RPC, revalidates GM rights and both faction keys on the server, then calls Reforger's native `SCR_FactionManager.SetFactionsFriendly` or `SetFactionsHostile`. The native manager mirrors both directions, refreshes AI perception, broadcasts the editor notification, and its faction delegates serialize the relationship matrix for join-in-progress clients.

No Achilles/Ares source, textures, layouts, compositions, or assets were copied. Achilles is CC BY-NC-SA 4.0 while Bifrost's original work is APL-SA; this review therefore ports behavior only through original Enfusion code and installed Reforger contracts.

## Portability gate

A missing outcome was marked **portable now** only if the installed Reforger build exposes a supported contract, the complete gameplay mutation can be server-authoritative, current and joining clients receive the result, the feature fits an existing Bifrost surface, and no Arma 3/ACE/CBA/TFAR/Advanced Rappelling/content dependency is required.

An idea that could be rebuilt as a new subsystem is not a direct port. Runtime SQF evaluation, Arma 3-only assets, ACE/TFAR bridges, generic ownership transfer, and features whose complete Reforger authority/JIP contract is absent are classified as **not direct** rather than half-implemented.

## How Achilles executes

Achilles is an Arma 3/CBA expansion that contains Achilles code and its inherited Ares base. `CfgPatches` loads the nine addons. `CfgFunctions` registers SQF functions by tag and directory. `CfgVehicles` derives placed Zeus modules from a shared base whose `isGlobal = 2`; each leaf names a function. The curator places a logic, the configured function opens local UI or resolves selected/attached entities, and the function then mutates locally, calls locality-owned objects, uses `remoteExec`/`remoteExecCall`, or publishes variables. UI replacements hook curator display load/unload, object/group/waypoint events, and property dialogs. CBA registers the ten hotkeys. Scripted waypoint files are invoked by `CfgWaypoints`.

The common Achilles path is:

`config or curator event -> registered SQF function -> local dialog/selection -> locality call or remoteExec/publicVariable -> Arma 3 object/global state`

Bifrost's implementation rule is:

`native editor action -> Bifrost picker -> reliable owner-to-server RPC -> GM rights and input revalidation -> native server mutation -> native replication/JIP state`

Achilles does not define a `CfgRemoteExec` whitelist and contains 80 files with remote-execution calls plus 25 files with public-variable usage. Its Arma 3 design cannot be used as multiplayer proof for Reforger; each outcome must be independently rebuilt around Enfusion authority and replication.

## Package inventory

| Addon | Contents and execution role | Bifrost decision |
|---|---|---|
| `data_f_achilles` | Achilles icons and UI images. | Not copied; Bifrost visuals remain original. |
| `data_f_ares` | Ares icons and 66 packaged Arma 3 compositions. | Composition behavior exists; Arma 3 class compositions and licensed assets are not portable. |
| `functions_f_achilles` | 100 feature/common/replacement functions and five waypoint scripts. | Outcomes mapped below; SQF is not source-portable. |
| `functions_f_ares` | 22 Ares common/feature helpers. | Outcomes mapped below. |
| `language_f` | Localization for modules, waypoints, dialogs, settings, and notifications. | Used only to resolve canonical feature names for this report. |
| `modules_f_achilles` | Achilles module configs and 61 implementation functions. | Every visible module classified below. |
| `modules_f_ares` | Ares module configs, 72 implementation functions including 50 extension slots. | Every visible module classified below. |
| `settings_f` | CBA settings and ten keybinds. | Mapped to native/Bifrost input surfaces; CBA is unavailable. |
| `ui_f` | 33 functions plus curator display/property replacements, dialogs, events, and hints. | Outcomes mapped to Bifrost's existing shell and native attributes. |

## Achilles module-by-module crosswalk

Status meanings: **Bifrost** is an existing authored equivalent; **Native** is a shipped Reforger outcome Bifrost preserves; **Ported** was added by this review; **Partial** is useful overlap without claiming feature identity; **Not direct** requires unavailable dependencies/content or a new unproven subsystem.

| Category | Every visible Achilles feature | Execution and outcome | Bifrost/Reforger result |
|---|---|---|---|
| ACE | Heal; Immersive Heal; Injury | Detects ACE or other medical frameworks and calls their treatment/injury functions on selected units. | ACE's Arma 3 API does not exist. Native Reforger health/property paths are separate. **Not direct.** |
| Arsenal | Add Full Arsenal; Copy to Clipboard; Create Custom; Paste Inventory; Remove Arsenal | Mutates virtual-arsenal arrays, serializes inventory to clipboard, or opens a custom item dialog. | Bifrost Arsenal Access, carried inventory, Gunsmith, Kits, reset, and server loadout application cover the operational outcomes. **Bifrost.** |
| Behaviours | Ambient Animation; Change Abilities; Change Altitude; Chatter; Have a Seat; Patrol/Loiter; Set Suicide Bomber; Surrender Unit | Dialogs choose animations/AI flags/altitude/chat/patrol behavior; functions run on selected units or their locality owner. Suicide Bomber adds an explosive behavior and damage handlers. | Animations FX, native behavior/speed/stance controls, chat, Patrol, Loiter, and server surrender/restore exist. Reforger has no equivalent generic Arma 3 AI-ability flag set or fixed-flight-altitude contract. Suicide Bomber needs a new explosive subsystem. **Bifrost + Native; remaining items Not direct.** |
| Buildings | Damage Buildings; Lock doors; Toggle lamps | Finds buildings/doors/lamps in the module radius and applies damage or animation/light state. | Bifrost server world controls open/close selected doors and toggle selected lights; authoritative FX/damage can affect buildings. There is no generic percentage-based building-damage editor. **Bifrost + Partial.** |
| DevTools | Advanced Composition; Bind Variable to Object; Function viewer; Show in anim viewer; Show in config | Opens composition management or Arma 3 developer/config viewers and binds missionNamespace variables. | The composition library is present. Runtime variable binding and Arma 3 viewers are unsafe or Workbench-only concerns. **Bifrost + Not direct.** |
| Effects | Light Source; White/Blue/Red/Green/Yellow Light; Persistent Smoke Pillar; Vehicle Fire; Small/Medium/Large Oily Smoke; Small/Medium/Large Wood Smoke; Small/Medium/Large Mixed Smoke | Creates local/global particle or light effect modules with attributes and persistent emitters. | Bifrost has replicated FX emitters, explosion, mortar, tracer, ambient, and loiter presentation. Achilles particle/light resources are Arma 3 content and cannot be copied; adding equivalent authored effects is new asset work. **Partial; content-specific variants Not direct.** |
| Environment | Advanced Weather Change; Earthquake; Set date | Dialogs drive Arma 3 weather/date commands; Earthquake creates camera shake, sounds, and effects. | Bifrost exposes native scenario time/date/weather controls. Earthquake requires a new replicated presentation/damage package. **Native + Bifrost; Earthquake Not direct.** |
| Equipment | Attach/Detach Effect | Attaches or removes supported effect objects from selected units. | Bifrost loadout/attachment editing and authoritative object attach cover supported outcomes. **Bifrost.** |
| Fire Support | Advanced CAS; Atomic Bomb; Create Target; Suppressive Fire | CAS scripts create and steer aircraft attacks; Nuke spawns Arma 3 effects/damage; target creates a universal target; suppression commands selected units. | Loiter/Air Support, target markers, artillery/mortar, and exact `Suppressive Fire` exist. Atomic Bomb requires content and a new large-area damage/presentation feature. **Bifrost + Native; Atomic Bomb Not direct.** |
| Mission Flow | Change Side Relations; Create/Edit Intel | Server `setFriend` changes both side pairs; Intel adds hold actions, diary records, markers, and notifications. | **Change Side Relations was Ported** using native replicated faction-manager state. Bifrost Markers & Intel and five briefing sections cover intel. |
| Objects | Attach To; Change height; Create IED; Enable/Disable Simulation; Equip with IED jammer; Hide Objects; Make invincible; Rotate Objects; Transfer ownership | Applies object transforms/attachment/global visibility/simulation/damage flags; IED installs trigger/damage handlers; transfer ownership moves Arma 3 network locality. | Precise transform, attach/detach, pause/simulation, visibility, invulnerability, and native replicated edits exist. Enfusion gameplay remains server authoritative, so generic ownership transfer is rejected. IED/ECM require a new explosive/jammer design. **Bifrost + Native; IED/ECM/ownership Not direct.** |
| Player | Set Radio Frequencies | Calls TFAR short/long-range frequency APIs. | TFAR is not a Reforger dependency. **Not direct.** |
| Reinforcements | Supply Drop | Chooses aircraft/cargo/approach and executes a parachute delivery. | Bifrost can create supplies and direct native load/unload and QRF/logistics orders, but has no complete aircraft/parachute supply-drop system. **Partial; complete feature Not direct.** |
| Replacement | Remote Control; Mine; CAS Gun; CAS Missiles; CAS Gun + Missiles; CAS Bomb Strike | Replaces selected vanilla curator modules/functions with Achilles variants. | Remote control, explosive placement, and fire support are already native/Bifrost surfaces. Arma 3 replacement hooks do not exist. **Native + Bifrost.** |
| Spawn | Advanced Composition; Mines/Explosives; Spawn Effect; Spawn Empty Object; USS Freedom; USS Liberty | Opens class/composition pickers and creates Arma 3 objects/effects/ships. | Bifrost CREATE, compositions, and FX cover generic outcomes. The two Arma 3 ship assets are unavailable. **Bifrost; ships Not direct.** |
| Zeus | Advanced Hint; Promote to Zeus; Switch unit | Broadcasts hints, creates/deletes curator logic, or swaps controlled units. | Bifrost notifications/chat plus Reforger's native GM rights and remote-control paths cover these outcomes. **Native + Bifrost.** |

## Inherited Ares module-by-module crosswalk

| Category | Every visible Ares feature | Execution and outcome | Bifrost/Reforger result |
|---|---|---|---|
| Behaviours | Garrison Building (instant); Un-Garrison; Search Building; Search and Garrison Building | Resolves the group under cursor and runs search/garrison logic on its owner. | Bifrost server-surveys building interiors and provides Garrison, Release Garrison, and directed Clear Building. **Bifrost.** |
| Custom | User Defined Module 0 through User Defined Module 49 | Fifty numbered slots call mission-supplied callback code. | These are extension slots, not fifty shipped outcomes. Runtime arbitrary code is not accepted; compiled native placeables/actions are the Reforger extension model. **Not direct.** |
| DevTools | Copy mission SQF; Execute Code Module | Serializes placed content to SQF or compiles and executes entered code. | SQF and runtime code compilation are unavailable and unsafe for a multiplayer GM surface. **Not direct.** |
| Equipment | Add NVD/Tactical Light/IR; Add/Remove Turret Optics; Toggle Tactical Light/IR Laser | Adds inventory/weapon items or toggles Arma 3 light/laser selections. | Bifrost Arsenal/Gunsmith handles supported gear and attachments; vehicle turrets retain authored optics. **Bifrost + Partial.** |
| Fire Support | Artillery Fire Mission | Selects artillery, ammunition, rounds, ETA/spread, then remote-calls firing commands. | Native Artillery fire plus Bifrost mortar/strike FX cover supported outcomes. **Native + Bifrost.** |
| Player | Change side of player; Create teleporter; Teleport; Bootcamp Stage; Punishment | Mutates player side/position, creates paired portals, or exposes selected vanilla modules. | Player faction properties and exact server-authoritative Teleport Players exist. Persistent portals are a separate entity system; Bootcamp/Punishment are Arma 3 modules. **Native + Bifrost; portal Not direct.** |
| Reinforcements | Create new LZ; Create new RP; Spawn Units | Stores marker positions and creates selected reinforcement groups. | Bifrost LZ/RP markers, CREATE, QRF staging, and group placement cover the outcomes. **Bifrost.** |
| Spawn | Submarine; Trawler | Spawns Arma 3 watercraft classes. | Those classes are unavailable. **Not direct.** |
| Zeus | Add/Remove editable Objects; Hide Zeus; Hint; Switch side channel of Zeus | Uses curator APIs, global hide/simulation/damage, hints, and channel changes. | Native editable registration/GM visibility plus Bifrost notifications/chat cover supported outcomes. Side-channel reassignment is tied to Arma 3 chat. **Native + Bifrost.** |

## Waypoint inventory

| Achilles waypoint | How it produces the result | Bifrost/Reforger result |
|---|---|---|
| Fastroping | Detects ACE or Advanced Rappelling and runs the dependency's helicopter rope sequence. | Dependencies/API absent. **Not direct.** |
| Land | Creates scripted helicopter landing behavior around an Arma 3 landing position. | Installed Reforger editable waypoints contain no generic Land prefab. A safe aircraft landing order needs a proven vehicle-AI contract. **Not direct.** |
| Paradrop | Server ejects passengers and invokes a parachute helper at the waypoint. | No equivalent installed editable waypoint or verified generic chute/eject contract. **Not direct.** |
| SearchBuilding | Runs Ares building search for the waypoint group. | Directed Clear Building already provides the server-authoritative outcome. **Bifrost.** |
| Repair | Searches for repair targets/sources and scripts a repair sequence. | Vehicle Service is player-operated; installed editable waypoints contain no generic Repair prefab. **Not direct.** |

Achilles also expands the waypoint property grid to Move, Cycle, Seek and Destroy, Hold, Sentry, Search Building, Get Out, Unload, Transport Unload, Land, Fastroping, Paradrop, Sling Load, Unhook, Repair, and Demine, with combat mode, speed, and timeout attributes. Bifrost imports the installed Reforger command set and exposes the omitted native Scout area, Wait, Load supplies, Unload supplies, and Suppressive Fire prefabs; the installed archive contains no Achilles-only waypoint equivalents.

## Curator UI, attributes, and keybinds

Achilles replaces the curator display lifecycle and handles object/group/waypoint placed, edited, deleted, double-clicked, and key events. It adds module-tree search and spawn attributes for Unit/Vehicle/Group, Include crew, and Specify position. Bifrost already supplies searchable/paged CREATE and EDIT trees, crewed-vehicle placement, native action preservation, its own context menu, overlays, and responsive layouts; duplicating the Arma 3 display hierarchy is neither possible nor desirable.

The property replacements expose:

- unit: Name, Rank, stance, Damage, Ammo, Skill, Respawn Position, Init/Exec; plus Skill, Arsenal, and Accessory buttons;
- group: Group ID, Skill, Formation, Behaviour, Combat Mode, Speed, stance, Respawn Position; plus Skill and Side buttons;
- vehicle: Ammo, Skill, Lock, Headlight, Engine, Respawn, Init/Exec; plus Garage, Cargo, Loadout, Damage, Sensors, and Accessory buttons;
- waypoint: type, combat mode, speed, and timeout;
- date: date/daytime with a preview control.

These map to native Reforger properties plus Bifrost Orders, Precise, Arsenal, Vehicle Service, world controls, waypoint radius/approach, and scenario date/weather. Arma 3 Init/Exec, pylon/garage material mutation, and scripted sensor flags are not portable generic attributes.

The ten CBA actions are Eject Passengers; Group Objects; Ungroup Objects; Deep Copy; Deep Paste; Countermeasure; Increase NVG Brightness; Decrease NVG Brightness; Toggle Include Crew; and Chatter. Their outcomes map respectively to unsupported parachute ejection, native/Bifrost grouping, the composition library, authored vehicle countermeasures, client display settings, CREATE's crewed toggle, and GM chat. CBA bindings themselves are not portable.

## Composition inventory

The Ares data addon ships 66 Arma 3 class-based compositions in these subcategories:

- Ares Military Structures: Bases (3), Bunkers (2), Camps (2), Field Support (4), Minefields (5), Misc (1), Road Checkpoints (7).
- Ares Walls: Composite Walls (2), Improvised Barriers (5), Wall Sections (7).
- Vernei community structures: Bunkers & Fighting Positions (5), FOBs & Bases (8), Mortar Positions (4), Radar Comms & Support (9), Roadblocks & Checkpoints (2).

Their `Land_*` class names, placement data, and license-bound authored arrangements cannot be copied into Reforger. Bifrost instead captures the GM's currently selected loaded-mod entities by resource name and relative transform, persists name/category/author metadata on the server profile, and recreates them through validated native placement. This directly satisfies the reusable-hill-base scenario without importing Achilles content.

## Function inventory and execution coverage

The companion [function inventory](ACHILLES_DEEP_REVIEW_FUNCTION_INVENTORY_2026-09-04.md) lists every one of the 294 `fn_*.sqf` files by addon/category. The visible module tables above cover all configured module outcomes. Helpers were classified through their consumer: selection/dialog/array/config helpers do not create separate user features; explosion, inventory, animation, garrison, CAS, IED, spawning, and curator helpers are represented by the module or UI surface that invokes them.

## Implemented `Change Side Relations` flow

1. On empty terrain, the Bifrost action menu exposes the exact label `Change Side Relations`.
2. The first picker lists every loaded faction from `DCO_FactionCatalog`; the second does the same and allows self-relations, matching the native manager's supported faction-infighting use case.
3. The final picker chooses `Friendly` or `Hostile` and sends only two faction keys plus the relation bit.
4. The server checks Game Master rights, rejects empty/unknown faction keys, resolves both as `SCR_Faction`, and calls the native two-way manager method.
5. The manager updates both sides, refreshes AI perception, sends its standard editor notification, broadcasts the relation change, and serializes all friendly flags in `SCR_EditableFactionComponent.RplSave`; `RplLoad` restores them for JIP.

## Evidence chain

- Enfusion API search found `SCR_FactionManager.SetFactionsFriendly` and `SetFactionsHostile`, explicitly documented as replicated when the server calls them, and found the mirrored faction/editor APIs.
- The complete BI Wiki replication overview established the server-as-source-of-truth, reliable RPC, simulation/presentation, dedicated-server, and JIP/streaming constraints used by the implementation.
- PAC1CLI read the installed `SCR_FactionManager.c` and `SCR_EditableFactionComponent.c`. Lines 595-640 and 680-723 show two-way mutation, AI refresh, delegate broadcast, and notifications. The delegate's `RplSave`/`RplLoad` relationship matrix at lines 494-654 establishes JIP delivery.

## Validation

| Boundary | Result |
|---|---|
| Achilles baseline | Pinned to `f123656459cab7766aa40c32d5ee12d29ebadaae`; clone clean at audit. |
| Exhaustive static inventory | 662 files, 9 addons, 294 SQF function files, 100 non-custom config/module entries, 50 custom slots, 5 scripted waypoints, 66 compositions. |
| Workbench `WORKBENCH` validation | Passed with 0 errors and 14 installed/base obsolete warnings. An initial validation found the unsupported ternary expression in the new diagnostic; it was replaced and validation then passed. |
| Source/static review | Relation IDs remain below the native-action range; faction indices are bounded; empty/unknown keys fail closed; GM rights are checked on authority. |
| License boundary | No Achilles/Ares implementation or asset copied. |
| Cleanup | Host policy rejected recursive deletion of `C:\Users\Bryce\AppData\Local\Temp\codex-achilles-review-20260904`; no process remains attached to it and all orphaned research `git fsmonitor--daemon` processes were stopped. |
| Dedicated server | Architecture-ready, not runtime-tested in this pass. |
| Remote client | Reliable request route present, not runtime-tested in this pass. |
| JIP | Native relationship matrix path confirmed statically in installed source, not hands-on tested. |
| UI/controller | Picker uses Bifrost's existing paged context menu, not hands-on tested. |

Static compile and installed-source evidence do not equal release acceptance. Before release, test Friendly and Hostile transitions on a dedicated server from a remote GM, verify existing AI stop/start engagement, connect a new client after each change, test self-friendly/self-hostile, test a third-party faction, and confirm denial for a non-GM client.

## Primary sources

- [Achilles repository and README](https://github.com/ArmaAchilles/Achilles)
- [Pinned reviewed source](https://github.com/ArmaAchilles/Achilles/tree/f123656459cab7766aa40c32d5ee12d29ebadaae)
- [Achilles license](https://github.com/ArmaAchilles/Achilles/blob/f123656459cab7766aa40c32d5ee12d29ebadaae/LICENSE)
- [Arma Reforger Script API: SCR_FactionManager](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceSCR__FactionManager.html)
- [Arma Reforger Script API: SCR_EditableFactionComponent](https://community.bistudio.com/wikidata/external-data/arma-reforger/ArmaReforgerScriptAPIPublic/interfaceSCR__EditableFactionComponent.html)
- [Enfusion replication overview](https://community.bistudio.com/wiki/Arma_Reforger:Replication)
