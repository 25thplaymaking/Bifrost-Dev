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

- The operator identified the earlier native Workbench crash as a false positive and cleared it as a release concern. That is an operator determination; this release does not claim to have established the native crash's cause.
- After the Gunsmith input and barricade-scale corrections, Workbench `WORKBENCH` validation passed with zero errors and 14 existing base-game deprecation warnings at approximately 22:45 EDT on September 4, 2026. See the focused repair record for the source evidence and changes.
- The final static replication review checked server-authorized mission requests, replicated terrain/object/interaction state, composition/marker/intel snapshots, and local UI settings. Dedicated-server, remote-client, and JIP acceptance remain with the user.
- The final CREATE scrollbar, ellipsis, and monochrome presets await the user's interaction checks. Earlier local mission-panel regression evidence is recorded separately in Tests/Workbench/README.md.
- Weapon selection, attachment-slot activation and static barricade scaling also await the user's interaction checks. Character scaling remains unsupported and now returns an explicit explanation. No live reload or playtest was performed for these corrections.
- Workshop metadata reported 1.0.28 during preparation. No package was created or uploaded by this release operation, so package fingerprint and Workshop parity are not claimed.

## BI upload handoff

Publish this exact clean checkout as version **1.0.29** using the existing Bifrost addon. Use the 1.0.29 changelog as the change notes. The source fingerprint is embedded in Configs/Release/BifrostRelease.conf and the annotated Git tag. Keep the publisher's output directory separate from the source directory.

## Static preparation results

- 1,264 candidate files were checked after the follow-up repair; no empty files or missing metadata for new runtime resources were found.
- 396 metadata GUIDs are unique; all 92 referenced Bifrost editor attribute classes resolve.
- No merge debris or eager non-constant static initializers were found in the Game scripts. The whitespace check passed.
- Temporary Enfusion MCP handlers are absent. No new Workbench or helper process was launched.

## Follow-up repair

[Gunsmith input and GM Scale Object repair](GUNSMITH_SCALE_REPAIR_2026-09-04.md) records the reported targets, supported behavior, source findings and validation boundary.
