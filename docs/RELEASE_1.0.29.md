# Bifrost 1.0.29 release record

REQUIREMENTS
- Publish the current Bifrost release to GitHub main, with a matching version tag and release notes.
- Leave the Bohemia Workshop upload and further in-game testing to the user.
- Preserve the complete candidate, addon identity, and source fingerprint.

MINIMUM COMPONENTS NEEDED
- Existing Git repository, release manifest generator, and GitHub release.
- Existing compile evidence and static integrity checks.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No Workshop upload, new playtest, packaging automation, or force push.

PRIMARY RISKS
- GitHub and Workshop remain on different versions until the user's upload completes.
- Static and compiler evidence does not establish dedicated-server, remote-client, or join-in-progress behavior.

REQUEST INTERPRETATION
- The user authorized the main release and retained BI publishing. This supersedes the release skill's usual Workshop-first promotion order. The project's compile-first policy in TESTING.md applies.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Release version 1.0.29 on GitHub and leave the local source ready for the user's upload under addon GUID 6A0C2D6CE9809C6E.

## Validation boundary

- **Main publication is paused:** the final log inspection found a native access violation at 22:27:46 on September 4, 2026, immediately after creating the Arsenal/Vehicle Service item-list layout. The invalid read was at address 0x50. The native stack has no actionable script frame, so the cause is not established. No speculative production patch was made. This is newer than the successful script compilation and prevents a clean runtime release claim.
- The final production script changes passed Workbench validation with zero script errors and 14 warnings. The authoritative log records Game module loading at 22:21:07 on September 4, 2026. A subsequent validation attempt during release preparation lost its connection; the read-only compiler fallback still reports the prior pass. No further production script edits were made during release preparation.
- The final static replication review checked server-authorized mission requests, replicated terrain/object/interaction state, composition/marker/intel snapshots, and local UI settings. Dedicated-server, remote-client, and JIP acceptance remain with the user.
- The final CREATE scrollbar, ellipsis, and monochrome presets await the user's interaction checks. Earlier local mission-panel regression evidence is recorded separately in Tests/Workbench/README.md.
- Workshop metadata reported 1.0.28 during preparation. No package was created or uploaded by this release operation, so package fingerprint and Workshop parity are not claimed.

## BI upload handoff

Publish this exact clean checkout as version **1.0.29** using the existing Bifrost addon. Use the 1.0.29 changelog as the change notes. The source fingerprint is embedded in Configs/Release/BifrostRelease.conf and the annotated Git tag. Keep the publisher's output directory separate from the source directory.

## Static preparation results

- 1,263 candidate files were checked before this record's final edit; no empty files or missing metadata for new runtime resources were found.
- 396 metadata GUIDs are unique; all 92 referenced Bifrost editor attribute classes resolve.
- No merge debris or eager non-constant static initializers were found in the Game scripts. The whitespace check passed.
- Temporary Enfusion MCP handlers are absent. No new Workbench or helper process was launched.

## Native crash evidence

The existing Workbench session's `logs_2026-09-04_19-36-47/console.log` records `GRSA_ItemListPanel.layout` creation at 22:27:46.278, followed by `ENGINE (F): Crashed` at 22:27:46.365. Its `crash.log` records the access violation and an unresolved native stack. The adjacent `EditBoxFilterComponent used on invalid widget type` warning also occurs in native chat UI, so it is not sufficient evidence to assign the crash to the edit box. The dump remains in the user's Workbench log folder; it is not included in the release source.
