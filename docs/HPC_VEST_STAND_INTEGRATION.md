# H.P.C. vest stand integration

REQUIREMENTS
- Import the authored coyote/black H.P.C. stand, its textures, LODs and material into this Bifrost-Dev workspace using Enfusion MCP.
- Respect the user-authored stand transform in the shared Arsenal environment; position the selected vest on its shoulders and the selected helmet on its dome during customization.
- Preserve the authored stand transform across item switches, rotation, attachment refresh and teardown. Reuse the existing single selected-item preview.
- Frame the stand and vest, preserve rotate/zoom/focus, attachment changes and existing authoritative Wear/apply behavior.
- Validate resource builds, scripts, preview geometry and the rendered transition as far as the available runtime permits.

MINIMUM COMPONENTS NEEDED
- Existing Assets tree, a stand prefab, and the existing shared Arsenal environment.
- Existing BIA_WeaponStage clothing path and camera contract, with vest-specific support placement.
- Targeted regression evidence in the existing Tests/Workbench structure.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No new preview world, menu framework, addon identity, workspace, public release or Workshop upload.
- No hard dependency on a particular external vest mod.

PRIMARY RISKS
- The current workspace contains substantial uncommitted Arsenal renaming and fixes; preserve them.
- Skinned vest bounds and equipment origins can differ between mods; support placement must use actual preview bounds and be checked on representative vests.
- The stand must not occupy the weapon table when weapon customization is active.
- Preview entities must stay local to the existing private world; presentation changes must not bypass authoritative inventory application.

REQUEST INTERPRETATION
- Replace the vest-on-table presentation with an upright vest on the H.P.C. stand in the same Arsenal environment; keep the existing weapon presentation.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
Import the completed stand directly into Bifrost-Dev and integrate it into the existing vest customization view, including camera framing and lifecycle cleanup.

## Verified workspace

`C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/addons/Bifrost-Dev/addon.gproj`, GUID `6A0C2D6CE9809C6E`, ID `BifrostDev`. Confirmed by native project locate and Workbench log session `logs_2026-09-06_15-00-37`.

The current shared environment is `Prefabs/UI/GRSA_StageEnvironment.et` (the Arsenal environment described by the user).

## Stand placement and item presentation

The user's saved stand position is `0.342 0.388 -0.713`, with authored Y rotation `228.448` degrees after the requested 180-degree front-facing turn. The script captures the entity's full transform instead of overwriting it with a table-centre position. Drag rotation adds to that orientation; release restores the original transform.

The existing clothing-customization path recognizes vest areas and HeadCover areas. Vests use a stand-local support height of 0.549 m, placing them 3.5 cm higher on the authored shoulders. Helmets use the 0.7493 m dome crown plus 0.015 m shell clearance. While a vest is edited, the current draft helmet and its draft attachments are cloned locally onto the dome; direct helmet editing still uses the primary item view. Item offsets come from the native preview body's bounds, independently of mounted children. Long vest previews preserve their native preview scale unless a uniform reduction is needed to leave 1 cm of world-space clearance above the stand base; this affects the private display clone only, not equipped gear. The camera frames the vest, helmet and complete stand. Clothing hardpoint offsets are bounded to the visible item and use a wider front-facing focus distance so the camera cannot enter the garment or tabletop. Authoritative Wear/apply logic and the Soldier mannequin's complete draft outfit are unchanged.

## Validation

- Enfusion MCP registered and built the model, packed textures and material in this workspace. The redundant interior screw trace mesh was removed; exterior collision and black visual screws remain.
- Native script reload and WORKBENCH validation passed with zero errors; the loaded dependency set reports 17 existing warnings during reload.
- `Tests/Workbench/run_stand_placement_regression.py` returned **62 passed**, no failures, using native `InventoryItemComponent.CreatePreviewEntity` for PASGT vest and helmet in a private preview world.
- Checks cover authored stand position/direction, initial visibility, the raised vest and helmet support anchors, companion-helmet creation, native item scale, world-space base clearance, a non-unit full-transform fixture, full-stand camera bounds, bounded front-facing hardpoint focus, rotation, anchor stability with a mounted child, transform restoration, visibility/state cleanup and private-world release.
- The vest preview correctly resolves its upright worn mesh. The dropped-item mesh has different bounds and is not used to derive the production support placement.
- Result details are saved in `Tests/Workbench/stand_placement_result.json`. The child fixture tests placement stability, not native equipment-slot compatibility.
- Visual fit across every mod's equipment, rendered UI transitions, and multiplayer acceptance have not been established by this geometry test.

## Repeating the native placement test

Temporarily copy `BIA_StandPlacementProbe.c` into `Scripts/Game/EnfusionMCP` and `BIA_StandPlacementRegression.c` into `Scripts/WorkbenchGame/EnfusionMCP`. Reload through the native script endpoint, then run the Python runner from Tests/Workbench. It creates and releases its own private world and native item previews without opening, saving or changing the user's editor scene. Remove those two temporary script copies and reload production scripts afterward.
