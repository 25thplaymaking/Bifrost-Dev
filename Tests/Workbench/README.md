# Isolated mission-panel regression

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

Copy `DCO_MenuLayoutProbe.c` into the authoritative workspace's temporary `Scripts/Game/EnfusionMCP/` and `DCO_MenuLayoutRegression.c` into `Scripts/WorkbenchGame/EnfusionMCP/`. Use native script reload, then run `run_menu_layout_regression.py`. Expected: 49 checks, no failures. September 7: the actual mouse-down/up sequence reproduced seven failures before the fix (37 passes); the corrected backdrop and placement guards pass all 49 checks, including press consumption, releases over controls, right-button release, release-only dismissal, reopen and keyboard/gamepad click fallback. These are handler-level checks on real widgets, not physical input automation. This creates and removes the shipped GM and Gunsmith layouts; it verifies that position controls are absent and OPEN CONTENTS is a visible 58-pixel action fixed above the mount list, invokes handlers directly and uses synthetic mount entries. It does not reproduce physical clicks or apply a kit. Remove temporary copies after testing.

September 6 follow-up: the prior 38-check suite passed, including a real attachment tile configured for one press-time activation and exactly one dispatch. The updated 39th check covers the contents action inside the clothing mount rail and remains pending the next native run. The stand placement suite passed 62 checks, including the raised vest, companion helmet, bounded clothing focus and complete cleanup. Temporary handlers were removed before the final production-only validation. Physical third-party tile clicks and rendered clothing fit remain operator checks.
