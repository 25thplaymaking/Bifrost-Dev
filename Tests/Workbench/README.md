# Isolated mission-panel regression

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
