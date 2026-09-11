# Bifrost 1.0.30 release preparation

REQUIREMENTS
- Fix trigger-controlled audio start, stop and status using the replicated playback state.
- Return the actual automatic/manual teleporter mode when reopening endpoint settings.
- Reject a teleport arrival if the same character has boarded a vehicle before delivery; retain native movement synchronization and the existing respawn identity guard.
- Preserve the complete reviewed gameplay candidate and remove temporary Workbench helpers.
- Prepare a clean local release commit, matching version, notes and source fingerprint.
- Validate available compiler, resource and integrity checks; distinguish pending runtime evidence.
- Leave BI upload and GitHub publication to the operator.

MINIMUM COMPONENTS NEEDED
- Existing FX component, audio settings and mission edit reply.
- Existing release manifest generator and an isolated Git worktree.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No new dependencies, movement redesign, speculative latency fix, or live publication.
- Keep unrelated startup-investigation and Enfusion MCP documentation in the development checkout.

PRIMARY RISKS
- The operator confirmed local movement-speed and teleporter behavior and cleared the startup UI-delay report as a fluke. This is operator evidence, not an automated multiplayer test.
- Compiler validation does not establish dedicated-server, remote-client or JIP behavior.
- Development and prepared release paths must be distinguished before packaging.

REQUEST INTERPRETATION
- Complete the final-review fixes and local preparation as version 1.0.30, without rewriting the existing 1.0.29 release.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
Prepare an identifiable, clean candidate with the reviewed audio, settings and delayed-arrival defects corrected and an honest BI upload handoff.

## Corrections

Audio emitters now route the existing FX start/stop entry point directly to their replicated playback control, and report firing status from that same state. Trigger deactivation stops playback with the configured fade. Non-audio barrage behavior is unchanged.

Teleporter edit replies derive their activation selection from the replicated automatic/manual setting. Intel replies retain their audience scope. Both local and remote reply paths use the same resolved value.

The server teleport helper and client arrival handler now reject occupied vehicles before calling native teleportation. The arrival handler retains its replicated character identity check, so a delayed message cannot target a replacement character after respawn. RPC direction, reliability and native movement synchronization are unchanged.

## Verification and limits

- Native WORKBENCH validation after the delayed-arrival fix: zero errors and 14 existing base-game deprecation warnings.
- Follow-up RPL source review traced the server request, delayed completion, checked destination, guarded teleport helper and client arrival handler against native teleport behavior. No further confirmed defects were found in that path; this is source review, not multiplayer runtime evidence.
- Temporary Workbench handlers were already absent by the cleanup check. Post-cleanup native WORKBENCH validation also passed with zero errors and 14 existing warnings. No Workbench process was launched or restarted; the operator's session was preserved.
- Layout validation: 363 checks across 26 layouts, zero failures; two fault-injection checks passed.
- Audio/resource validation: all six file hashes, formats, graph structure, metadata and nine distinct audio controls passed.
- The audio and settings corrections were checked by following the complete server start/stop/status and local/remote settings-reply paths. No new play session or script reload was performed in the operator's running world.
- Operator confirmation on September 5: movement speed is fixed, teleporters work locally as expected, and the UI delay was a fluke. This supersedes the earlier open startup-latency concern and the local movement/teleporter handoff in the field-test record.
- Dedicated-server, remote-client, reconnect/JIP and listening-quality tests are not claimed.
- The existing GitHub 1.0.29 release is preserved; version 1.0.30 is a new local candidate. No tag, push, GitHub release or BI upload is performed by this preparation.
- Package validation must occur after Workbench creates the bundle. A source fingerprint is not proof of an unbuilt package.

## BI upload handoff

Use the clean release project at C:/Users/Bryce/Documents/Bifrost-Releases/1.0.30/addon.gproj, not the development checkout. Keep the publisher output outside both source directories. Publish under the existing addon GUID 6A0C2D6CE9809C6E as version 1.0.30 using APL-SA and the existing visibility settings.

Workshop was still at 1.0.28 during preparation. The BI change notes should include the 1.0.29 and 1.0.30 changelog sections because 1.0.29 was not uploaded there.

Before confirming upload, verify the bundled Configs/Release/BifrostRelease.conf matches this clean candidate's version and fingerprint. After BI succeeds, publish the same candidate commit to GitHub with a new v1.0.30 tag; do not move v1.0.29.
