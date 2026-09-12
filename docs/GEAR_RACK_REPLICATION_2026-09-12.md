# Gear-rack remote-client hotfix

REQUIREMENTS
- Diagnose disappearing gear and disabled Take actions using the live server container logs and deployed source.
- Check the reported Use Virtual Inventory setting; preserve original items and nested cargo through Hang/Take.
- Fix both cross variants, validate the native prefab configuration and replication paths, and prepare the GitHub hotfix for distribution.
- Keep the production server running; no restart, live script reload, package replacement or server configuration change. BI publication remains with the maintainer.

MINIMUM COMPONENTS NEEDED
- Existing rack prefab/storage configuration and existing transfer/display code only where evidence requires changes.
- One bounded native prefab regression; existing release checks and source metadata.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No speculative replacement replication framework or live-server experiment.

PRIMARY RISKS
- Virtual inventory can retain server contents without the remote entity required by display and Take eligibility.
- A source-checksum mismatch is also logged; it is not proof of the cause of disappearing gear.
- Compilation and configuration assertions do not establish remote/JIP acceptance.

REQUEST INTERPRETATION
- Prepare and validate an urgent rack hotfix, publish its identifiable source, and leave server rollout timing with the operator.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Restore visible, recoverable gear on dedicated-server racks without interrupting the current mission.

## Initial evidence

Authoritative workspace: `C:\Users\Bryce\Documents\My Games\ArmaReforgerWorkbench\addons\Bifrost-Dev`, ID `BifrostDev`, GUID `6A0C2D6CE9809C6E`; disk and native MCP location agree.

Read-only server inspection: `grain.silo`, container `reforger-08a3268f-f6ce-4c0b-a6ed-3f8bbbea9c1a`, started `2026-09-12T21:14:47.388962018Z`. Current log folder: `/opt/25vid/backend/server-data/08a3268f-f6ce-4c0b-a6ed-3f8bbbea9c1a/profile/logs/logs_2026-09-12_21-15-02`.

The server records two XL crosses at 21:18:54 and 21:18:56 UTC and the small cross at 21:18:58, followed by deletion of one XL at 21:19:27. No rack-specific VM or inventory failure is recorded in that incident interval. Script-checksum rejection occurs at 21:19:10 and again at 21:21:56; its relationship to the reported rack failure is unproven. The screenshot shows an empty cross offering disabled Take actions.

Deployed Workshop metadata and ServerData identify Bifrost **1.0.33**, updated 21:02:27 UTC, with a 71,155,784-byte data.pak. GitHub's preceding source release is v1.0.32; the hotfix must use a version above the deployed 1.0.33.

## Confirmed defect and correction

Native prefab inspection reports that `DCO_GearRackStorageComponent` inherits `UseVirtualInventoryReplication` with default **1**. Neither deployed rack prefab overrides it. The display path resolves a replicated stored-item ID to an entity before creating its local preview, and Take eligibility requires that entity to locate a compatible return slot. Virtual storage is incompatible with those assumptions on remote clients. The failure is consequently not visible in a local-only transfer test.

PAC1CLI compared the deployed package's small-cross prefab, XL prefab, `DCO_GearRack.c` and release manifest against the local v1.0.32 files; all four matched after newline normalization. Deployed package SHA-256: `e4d86bdf17749288d7974e41e4c454413b2691da8ff9ea7560f11349a165c429`. Its embedded source manifest still identifies v1.0.32 even though Workshop identifies the upload as 1.0.33. The temporary local package copy was removed after comparison.

Both saved rack prefab storages now explicitly set `UseVirtualInventoryReplication 0`. The maintainer's concurrent native prefab saves and XL override were preserved; their component/action ordering changes are serialization only. The shared parent covers small and inherited rack variants. The authoritative transfer implementation, original inventory items, geometry and replica-ID snapshot paths are unchanged. No replacement network messages or display-item copies were added.

## Verification and release identity

- Native configuration regression: **20 passed, zero failures**, including resolved settings for both variants, enabled RplComponent, retained small/XL slot support, and unchanged virtual inventory on a native ammunition box. The first assertion run passed 19 and identified the small cross still enabled after the maintainer's XL correction; the final saved/native state passes all 20. See `Tests/Workbench/gear_rack_replication_result.json`.
- The native control uses its verified resource GUID. The final fixture run produces no missing/invalid resource diagnostic. Earlier exploratory bare-path control loads logged a GUID warning; those were fixture-only and the final runner corrects that path.
- Temporary handler removed. Final production script reload and native WORKBENCH validation passed at **17:30 EDT**, **zero errors and 14 existing base-game obsolete-API warnings**, with successful game initialization. Log folder: `C:\Users\Bryce\Documents\My Games\ArmaReforgerWorkbench\logs\logs_2026-09-12_17-22-00`.
- Source integrity: 454 runtime files, 137 metadata GUIDs and 102 distinct referenced Bifrost attribute classes; zero failures. No runtime Game script changed in this hotfix.
- GitHub source release: **[v1.0.34](https://github.com/25thplaymaking/Bifrost-Dev/releases/tag/v1.0.34)**. Embedded manifest and README agree on 1.0.34. Runtime-source SHA-256: `c16cff1eb25eafdcd536c4d22c70c57fd8f6e52e67657f66e45485de3dbff961`, reproducible with `python Tests/verify_release.py --version 1.0.34`.

The production container was inspected read-only throughout. No restart, stop, command injection, package overwrite, runtime script reload or configuration change was sent to that server. Native reloads above were confined to the local Workbench.

## Operator rollout and acceptance

The maintainer publishes BI version **1.0.34** from this same authoritative workspace and GUID. GitHub publication does not hot-patch a running server. Leave the current mission running; arrange matching server/client builds at an operator-controlled update window. Preserve occupied racks while investigating recovery rather than deleting their stored gear. This change does not claim to repair already-instantiated virtual storage in a currently running old build.

After the matching build is installed, verify on a dedicated server with two clients:

1. Place fresh small and XL crosses. Empty supported slots offer Hang; Arsenal remains first. Hang a vest and helmet, plus a belt on XL. Gear leaves the body, appears on both clients and changes the relevant action to Take.
2. With a compatible body slot free, Take returns the original gear and changes the action back to Hang. An occupied return slot must explain refusal; freeing it enables Take. Repeat several cycles on each cross.
3. Stock a mounted vest/belt pouch two compartments deep with magazines and attach supported equipment. Hang/Take preserves the exact counts, attachments and condition; another client observes the same result.
4. Join after the rack is occupied, then move out of streaming range and return. The same displays and usable Take actions must recover. Check simultaneous Take from two clients yields one item, without duplication or a permanently busy rack.
5. Repeat with the reported third-party vest, helmet and belt. Check client and server logs for new rack/inventory/replication failures. A script-checksum rejection requires matching the complete installed mod set and build; this prefab fix alone does not resolve an unrelated checksum mismatch.

These remote-client, dedicated-server and JIP steps remain direct acceptance tests. The completed local native checks verify the configuration defect and its correction without interrupting production.
