# Isolated mission-panel regression

`DCO_MissionPanelRegressionProbe.c` exercises the production panel's snapshot replacement, selection, paging and reply-correlation methods using temporary records and no widgets. It does not modify the marker service, world or scenario. The probe derives from the panel only to expose its protected state and simulate whether its editor is open. `DCO_MissionPanelRegression.c` exposes the test through Workbench's local NET API.

To run against an already-open Workbench in edit mode:

1. Temporarily copy `DCO_MissionPanelRegressionProbe.c` into `Scripts/Game/EnfusionMCP/` and `DCO_MissionPanelRegression.c` into `Scripts/WorkbenchGame/EnfusionMCP/`. The probe must be in the Game module: WorkbenchGame cannot override sealed methods across the module boundary.
2. Reload game scripts through the Enfusion MCP/native NET API. This requires a responsive Workbench with no blocking engine dialog.
3. Run `python Tests/Workbench/run_mission_panel_regression.py`. It sends one request to the local NET API and expects 18 passing checks with no failures.
4. Remove both temporary copies before publication. The source tests stay here outside the shipped script modules.

Coverage includes original weak-reference destruction, independent display copies, filtering, reorder/rename, deletion of the chosen destination, repeated replacement snapshots, empty-list paging, stale/duplicate/current results, and replies after closing/reopening the editor. It does not establish network transport, dedicated-server, JIP or visual behavior.

September 4 follow-up: native script reload compiled the production code and test with zero errors and 14 existing base-game warnings. The local NET API runner returned `passed: 18` with no failures at 19:43. A subsequent local GM playtest saved two consecutive positions at 19:47:46 and 19:47:55 without the reported exception. This runtime evidence supersedes the earlier timed-out probe attempt. Temporary script copies are absent; post-cleanup WORKBENCH validation passed with zero errors and 14 warnings.
