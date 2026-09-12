# Isolated mission-panel regression

## Rack inventory replication configuration

`DCO_GearRackReplicationRegression.c` reads the two production prefab sources through native Workbench resource loading, including inherited component values. Temporarily copy it into `Scripts/WorkbenchGame/EnfusionMCP/`, use native script reload and run `run_gear_rack_replication_regression.py` in the authoritative Bifrost-Dev project. Expected: **20 passes, zero failures**. It creates no scene entities and makes no inventory or server changes. It checks that both racks disable `UseVirtualInventoryReplication`, retain enabled replication and their respective slot capabilities, and leave the native ammunition-box setting enabled.

The September 12 hotfix run passed all 20 checks; see `gear_rack_replication_result.json`. The first assertion run detected virtual inventory still enabled on the small cross while the maintainer had already disabled it on XL. After both native resources reflected the saved fix, all checks passed. The temporary handler was removed, and production reload/validation passed with zero errors and 14 existing base-game warnings. These checks establish native configuration and inheritance, not dedicated-server transport or remote/JIP presentation. See `docs/GEAR_RACK_REPLICATION_2026-09-12.md` for rollout and acceptance steps.

## Property-session lifecycle regression

Run only in a disposable Workbench edit-mode instance with no user play world. The probe advances the game call queue by 0.2 seconds to check cancelled callbacks after their due time. The lifecycle checks use no widgets; the appended lighting probe creates and releases a private preview world and its lights. It must not run in a live or user-owned session.

Temporarily copy `DCO_SessionLifecycleProbe.c` to `Scripts/Game/EnfusionMCP/` and `DCO_SessionLifecycleRegression.c` to `Scripts/WorkbenchGame/EnfusionMCP/`, then reload through the native `ReloadScripts` endpoint. Run `python Tests/Workbench/run_session_lifecycle_regression.py`. A pass requires 34 lifecycle checks, no failures, three valid private-world studio lights and audio-graph initialization. The additional checks exercise the production backdrop handler's press/release/click methods, outside cancellation without a manager end event, and immediate reopen. They do not synthesize actual pointer input or prove in-game focus routing. Remove both temporary copies afterward and validate production scripts again.

The probe exercises the production confirm/cancel cleanup, reopens before old category/conditional/time-selection callbacks are due, and checks shutdown cancellation. The engine retains cancelled queue entries until due; immediate GetRemainingTime is not proof that cancellation failed. This test covers state and callback execution, not visible widget behavior, dedicated-server delivery or remote rendering.

September 6 lifecycle result: 34 passed, with studioLights and audioGraph true. See docs/SESSION_ACTION_2026-09-06.md for current final-cleanup and compile status.

## Field-test resource probe

`DCO_FieldFeatureProbe.c` creates and removes a passive hint, an audio emitter and a standalone teleporter in a local play world. It checks widget dimensions and text, all six native sound events, three acoustic signals and the teleporter's editable target/attribute. It does not use or move existing scene entities. Run only in a disposable local test world with a viewport, never against a live multiplayer session.

1. Copy the probe into `Scripts/Game/EnfusionMCP/` and `DCO_FieldFeatureRegression.c` into `Scripts/WorkbenchGame/EnfusionMCP/` temporarily.
2. Reload using the native script reload and enter a local play world.
3. Run `python Tests/Workbench/run_field_feature_regression.py`. A complete pass requires 25 checks and no failures; no-world or no-viewport runs cannot pass. Six checks create native playback handles beside the listener at low gain and terminate them immediately; they establish event playback setup, not listening quality.
4. Remove temporary copies when finished. Source tests remain in this directory.

`DCO_FieldResourceValidation.c` is a separate temporary Workbench handler that requests native PC builds for the 12 changed audio, hint, teleporter and settings resources. Its `requested: 12` reply only counts requests: inspect the Workbench console for each resource's build success and post-load diagnostics before calling it a pass.

Run `python Tests/verify_field_resources.py` and `python Tests/verify_feature_layouts.py` for the independent file/hash/metadata and UI binding checks. These are not engine or multiplayer tests.

## Mission-panel probe

`DCO_MissionPanelRegressionProbe.c` exercises the production panel's snapshot replacement, selection, paging and reply-correlation methods using temporary records and no widgets. It does not modify the marker service, world or scenario. The probe derives from the panel only to expose its protected state and simulate whether its editor is open. `DCO_MissionPanelRegression.c` exposes the test through Workbench's local NET API.

To run against an already-open Workbench in edit mode:

1. Temporarily copy `DCO_MissionPanelRegressionProbe.c` into `Scripts/Game/EnfusionMCP/` and `DCO_MissionPanelRegression.c` into `Scripts/WorkbenchGame/EnfusionMCP/`. The probe must be in the Game module: WorkbenchGame cannot override sealed methods across the module boundary.
2. Reload game scripts through the Enfusion MCP/native NET API. This requires a responsive Workbench with no blocking engine dialog.
3. Run `python Tests/Workbench/run_mission_panel_regression.py`. It sends one request to the local NET API and expects 18 passing checks with no failures.
4. Remove both temporary copies before publication. The source tests stay here outside the shipped script modules.

Coverage includes original weak-reference destruction, independent display copies, filtering, reorder/rename, deletion of the chosen destination, repeated replacement snapshots, empty-list paging, stale/duplicate/current results, and replies after closing/reopening the editor. It does not establish network transport, dedicated-server, JIP or visual behavior.

September 4 follow-up: native script reload compiled the production code and test with zero errors and 14 existing base-game warnings. The local NET API runner returned `passed: 18` with no failures at 19:43. A subsequent local GM playtest saved two consecutive positions at 19:47:46 and 19:47:55 without the reported exception. This runtime evidence supersedes the earlier timed-out probe attempt. Temporary script copies are absent; post-cleanup WORKBENCH validation passed with zero errors and 14 warnings.

## Native vest mounts

Load the candidate with the installed GRS Vests and Rigs (651834C8D77BF86B) and Minnesinger Gear (68CEC58B288B3AC0) plus their dependencies. They are test dependencies only; do not add them to Bifrost's addon.gproj. The base game must be discoverable alongside the installed mod directory.

Temporarily copy DCO_VestMountProbe.c into Scripts/Game/EnfusionMCP and DCO_VestMountRegression.c into Scripts/WorkbenchGame/EnfusionMCP. Reload through native ReloadScripts, start a disposable local game and let startup finish. Run python Tests/Workbench/run_vest_mount_regression.py. The expanded suite expects 24 checks with both mods loaded, or 13 with an explicit GRS skip when only Minnesinger is loaded. No failures are allowed. It requires the initialized game's preview manager; edit-mode results cannot establish compatibility. The expanded capacity checks still need a complete runtime run.

The test uses actual GRS MFCR and Minnesinger FCPC resources in a private preview world, including native slot restrictions, pinned attachment, duplicate pouches, storage-level movement/removal, incompatible pins and invalid-ID preservation. Saved-kit fixtures cover exact duplicate pins, native storage subclasses with nested mounts, an explicit empty draft, and secondary-store isolation. The UI no longer exposes position movement. This test does not establish dedicated-server application or observer/JIP appearance. Remove all temporary copies afterward.

September 6 vest result: 18 passed, no failures. Temporary copies were removed; production-only validation passed with zero errors and 17 base/dependency warnings. Rendered UI and dedicated-server acceptance remain open.

## Actual menu widget regression

Copy `DCO_MenuLayoutProbe.c` and `DCO_PropertyScopeProbe.c` into the authoritative workspace's temporary `Scripts/Game/EnfusionMCP/` and `DCO_MenuLayoutRegression.c` into `Scripts/WorkbenchGame/EnfusionMCP/`. Use native script reload, then run `run_menu_layout_regression.py`. Expected: 838 checks, no failures (153 widget/input assertions and 685 property-scope assertions). The earlier runner expected 840; that stale total was corrected after counting the current saved fixture branches independently. The runner requires edit mode with no active GM. The property checks create four local prefab fixtures in a private preview world, then delete them and release the world. They check all 110 registered Bifrost attributes, mixed selections, localization, native rich-text bindings and empty-session transitions. These are handler-level checks on real widgets, not physical input automation. This creates and removes the shipped GM and Gunsmith layouts; it verifies that position controls are absent and OPEN CONTENTS is a visible 58-pixel action fixed above the mount list, invokes handlers directly and uses synthetic mount entries. It does not reproduce physical clicks or apply a kit. Remove temporary copies after testing.

September 12 follow-up: all 155 checks passed in `C:\Users\Bryce\Documents\My Games\ArmaReforgerWorkbench\addons\Bifrost-Dev`, GUID `6A0C2D6CE9809C6E`. The added checks cover Properties handoff with data-only attributes, orphan backdrops, missing manager completion, all 12 floating-panel hit areas, modal/action-listener agreement, destroyed native ownership, close-gesture suppression, preserving another window's focus, native close lifecycle events, late native close callbacks, menu reopening during an action, disabled/stale actions, drag/resize interruption, native fallback dropdown cleanup, box-selection press/hold/release and delayed confirmation, post-modal normal/Ctrl recovery, blocked right-release state reset, and trigger render cancellation when release is lost. Nine new assertions failed before the follow-up production fixes and passed afterward. Drag fixtures use an unsaved panel name so they do not change user geometry preferences. The probe does not advance the global call queue or start a play world. Temporary handlers were removed, native production reload succeeded, and final validation returned zero errors and 14 base-game obsolete-API warnings. Physical input, remote-client, dedicated-server and JIP acceptance remain separate. See `docs/GM_INPUT_LOCKUP_2026-09-12.md`.

September 7 historical result: the mouse-down/up handler sequence reproduced seven failures before that fix (37 passes); the corrected backdrop and placement guards passed all 49 checks, including press consumption, releases over controls, right-button release, release-only dismissal, reopen and keyboard/gamepad click fallback.

September 6 follow-up: the prior 38-check suite passed, including a real attachment tile configured for one press-time activation and exactly one dispatch. The updated 39th check covers the contents action inside the clothing mount rail and remains pending the next native run. The stand placement suite passed 62 checks, including the raised vest, companion helmet, bounded clothing focus and complete cleanup. Temporary handlers were removed before the final production-only validation. Physical third-party tile clicks and rendered clothing fit remain operator checks.


## Placeable Arsenal rack and nested cargo

`BIA_ArsenalRackProbe.c`, `BIA_ArsenalRackRegression.c` and `run_arsenal_rack_regression.py` are the September 12 rack suite. Temporarily copy the probe into Scripts/Game/EnfusionMCP and the handler into Scripts/WorkbenchGame/EnfusionMCP in the authoritative project, then use native script reload. Run only after a local GM_Arland game is fully initialized, with Arsenal closed. It creates transient local fixtures in the active test world and removes them. It checks native inventory identity, ALICE mounted pouch stocking and capture/apply, invalid mounts, local displays and opaque UI surfaces. The XL extension requires the six actual CDD/Minnesinger belt prefabs and ZEL Waist slot, and checks Hang/Take with at least one real magazine in a nested belt pouch. The runner rejects a stale suite without the XL completion marker and rejects every reported failure.

The completed initialized-runtime suite passed 242 checks with no failures. It uses an asynchronous fixture sequence and waits for native storage completion, checks Hang/Take labels and original identity, and rejects stale or incomplete results. This run predates the final vest/belt pose adjustment. The subsequent native pose suite passed 116 checks with no failures. These results do not establish dedicated-server, remote-client, JIP or final rendered visual acceptance. Remove all temporary source copies after use. See docs/PLACEABLE_ARSENAL_GEAR_REVIEW_2026-09-12.md for the exact verification boundary and reproduction actions.

## Gear Cross support geometry

Temporarily copy BIA_StandPlacementProbe.c to Scripts/Game/EnfusionMCP and BIA_StandPlacementRegression.c to Scripts/WorkbenchGame/EnfusionMCP, then reload scripts. Run run_stand_placement_regression.py while the editor is idle. The test creates and releases a private world without changing the user's scenario. The runner requires at least 120 passes and no failures. It covers the Arsenal stand and both real world rack prefabs, PASGT shoulder collision, ALICE shoulder pieces, unchanged stock helmets, changed stand transforms, attachment clearance and hierarchy cleanup.

The third-party section uses 72 canonical prefab resources from the loaded CDD, Minnesinger and RHS addons, with native loadout areas including ZEL. Missing optional resources are explicitly skipped; a base-only pass cannot establish third-party coverage. The final September 12 run loaded all 72 representatives (43 carriers, 23 helmets, six belts) and passed 595 checks with no failures. It checks tilted/scaled non-origin stands, reversed worn-mesh axes, complete shoulder roots versus auxiliary armour, flat damage-plate helpers, belt bands versus suspended harnesses, and body contact. Visual-only shoulder geometry uses an estimate and still requires rendered-fit acceptance.

The result is stand_placement_result.json and names the authoritative workspace and GUID. Remove both temporary source copies and validate production scripts afterward. This suite does not establish physical menu clicks, dedicated-server transport, remote observation or JIP; use docs/PLACEABLE_ARSENAL_GEAR_REVIEW_2026-09-12.md for the combined operator tests.

## Soldier first-press browser

BIA_SoldierBrowserProbe.c and BIA_SoldierBrowserRegression.c are the focused September 12 category regression. Temporarily copy them into Scripts/Game/EnfusionMCP and Scripts/WorkbenchGame/EnfusionMCP respectively, compile, and send the local NET API request APIFunc BIA_SoldierBrowserRegression using the existing packed/receive_string helpers. The final release run passed 52 checks with no failures (soldier_browser_result.json).

The probe uses the shipped Soldier layout and production category creation/list-opening handlers. It temporarily supplies an empty draft, restores the previous service and focus synchronously, and removes its widgets. It checks first press, immediate correct-panel visibility, duplicate release suppression, Back, disabled rows and native controller activation. It does not claim physical input or multiplayer acceptance. Remove both temporary module copies and compile production scripts afterward.

The completed extension covers populated PASGT draft equip/remove, duplicate release suppression, disabled/controller item selection and quantity stepper isolation. Ten further checks cover matching, stale and duplicate Wear replies, edits after sending, failed/partial results, and reopened-session request identity. The September 12 release run passed all 52 checks. The wider menu/property suite passed all 838 current assertions with no failures. Temporary module copies were removed; production native reload and WORKBENCH validation passed with zero errors and 14 existing base-game warnings at 16:58 local time in the authoritative Bifrost-Dev workspace.
