# Bifrost 1.0.32 — GitHub source release and BI handoff

Release: [v1.0.32](https://github.com/25thplaymaking/Bifrost-Dev/releases/tag/v1.0.32), September 12, 2026. The maintainer handles Bohemia Workshop publication. No BI upload or external addon copy was made by this task.

## Identity and source boundary

All implementation and validation use the existing authoritative workspace:

`C:\Users\Bryce\Documents\My Games\ArmaReforgerWorkbench\addons\Bifrost-Dev`

Native MCP project location and disk identity agree: `addon.gproj`, ID `BifrostDev`, title `Bifrost-Dev`, GUID `6A0C2D6CE9809C6E`. The only declared dependency is base-game data `58D0FB3206B6F859`. Preserve this project and GUID for BI publishing; the older external-copy instructions in `RELEASE_1.0.30.md` are historical and do not apply.

The GitHub tag identifies the complete source commit. `Configs/Release/BifrostRelease.conf` records this runtime-source SHA-256:

`68328d6066f107d2529f4a8aa288a2915c8663a0a8780dace89131a10a93229c`

`python Tests/verify_release.py --version 1.0.32` checks the Git index and reproduces that fingerprint. Its input is each sorted tracked runtime path, a NUL, the canonical Git blob length as eight little-endian bytes, then the blob. Runtime roots are Assets, Configs, Prefabs, Scripts/Game, UI and Sounds; the manifest itself is excluded to avoid self-reference. This is a source fingerprint, not a Workshop package hash.

GitHub previously published v1.0.30. The historical Workshop 1.0.31 observation has no recoverable immutable source baseline in the reviewed repository, so this release does not invent a 1.0.31 comparison. Server and clients must use the same newly built version when testing replication.

## Changes and cleanup

- Both Arsenal gear categories and item selections activate on the first press; quantity controls retain their independent native handling.
- Obsolete Arsenal layouts and the former overlapping UI implementation are removed. Opaque surfaces preserve text contrast against stage lighting.
- Lightning > Bifrost includes the small Placeable Arsenal and XL Gear Cross. Arsenal is first; world actions alternate Hang/Take for vest and helmet, plus belt on XL. Rack actions are absent from the Arsenal screen.
- Runtime stand fitting uses support and clothing geometry for stock and available third-party gear. Optional gear addons remain optional.
- Nested stocking and pinned attachment compatibility are checked before preview or authoritative application.
- Properties retain their own lifecycle, scope object settings to supported selections, and use rich text or cleaned plain labels as appropriate. Modal and floating controls block conflicting world input.
- Routine pause, layout, tactics, catalog and effects diagnostics are removed. Failure messages, intentional performance tools and explicitly enabled debugging remain. Temporary probes are absent from runtime modules; the installed MCP bridge and machine-specific configuration are ignored by Git.

## Replication review

| Path | Authority and shared state |
| --- | --- |
| Rack Hang/Take | Reliable request on the player's owned controller. The server derives the controlled character, validates range, life state, slot compatibility and rack availability, then moves the original native inventory entity. Per-player and per-rack busy state serialize competing operations. Completion, failure and rack deletion release the requester. |
| Rack displays and JIP | Replicated vest/helmet/belt IDs and contents revision trigger local refresh. RplSave/RplLoad preserve the stored-item IDs; unresolved inventory entities retry after streaming. Preview copies are local and skipped on dedicated servers. Original cargo, damage and attachments remain on native inventory entities. |
| Wear and nested loadouts | Reliable bounded JSON stream and server validation retain policy, character, GM/access, proximity and compatibility checks. Begin/chunk/apply/result messages now carry a request ID. The client acknowledges only its current request and unchanged draft revision, including across reopened Arsenal sessions. Old/duplicate replies cannot clear newer edits. |
| Properties and GM actions | UI changes select/configure existing server-owned actions. Editor input and display geometry remain local; shared gameplay continues through authoritative relays, replicated properties and existing snapshots. |

Source review and isolated handlers do not establish physical dedicated-server, remote-client or JIP behavior. Those remain separate acceptance levels.

## Verification evidence

- User confirmed the repaired local Arsenal gear and item selection workflow before release cleanup.
- Native Soldier browser/Wear suite: **52 passed, zero failures**, including populated PASGT selection, release suppression, disabled/controller activation, quantity isolation, stale/current/duplicate replies and reopened sessions. See `Tests/Workbench/soldier_browser_result.json`.
- Native menu/Properties suite: **838 passed, zero failures**. The executable fixture has 153 widget/input assertions plus 685 Properties assertions; independent branch counting corrected the runner's stale 840 expectation without removing or weakening an assertion. See `Tests/Workbench/menu_layout_result.json`.
- Temporary test-module copies removed. Production native reload and WORKBENCH ValidateScripts passed at approximately **16:58 local**, with **zero errors and 14 pre-existing base-game obsolete-API warnings**. Logs: `C:\Users\Bryce\Documents\My Games\ArmaReforgerWorkbench\logs\logs_2026-09-12_16-38-43`. The final reload initialized the game successfully; no fresh candidate-caused VM, GUI, replication or resource error was observed. The fixture runs reproduce the existing native EditBoxFilter warning.
- Static checks: **360 bindings across 25 layouts, zero failures, two fault-injection checks passed**; all **15 attribute layouts** covered; six audio files and source hashes, graphs, metadata and field-resource syntax passed.
- Release integrity: **454 runtime files, 137 metadata GUIDs, 102 distinct referenced Bifrost attribute classes**, zero missing metadata, duplicate GUIDs, empty runtime files, unresolved attribute classes, merge debris or eager nonconstant static initializers.
- Earlier same-task native results remain separately scoped: rack/nested inventory **242 passed**; stand geometry **595 passed** with 72 third-party representatives. These are recorded in `PLACEABLE_ARSENAL_GEAR_REVIEW_2026-09-12.md` and the corresponding test results; they were not relabeled as dedicated-server acceptance.

Installed engine sources were reviewed through PAC1CLI for native button activation, editor attributes, placement, inventory slots and replication behavior. MCP supplies engine validation, not a product certification.

## Combined operator checks after the BI build

1. Open Arsenal on a stock soldier and select Headgear, Vests, Armored Vests and Footwear once each. Select, remove and reselect an item once per gesture. Enter Contents and return with Back/Escape; the correct category must appear immediately. Quantity controls must change cargo without selecting their parent row.
2. Send Wear, then immediately edit another item before the reply; the new edit must remain pending. Repeat while closing/reopening Arsenal and with a partial or refused apply. The new session must not report the old request as its own.
3. Place both crosses from Lightning > Bifrost. Hang/take stock PASGT and ALICE gear, a helmet and an XL belt. Verify shoulder contact, correct helmet pose, alternating actions, original item condition and unchanged nested magazines. Repeat with the intended third-party gear stack.
4. Load magazines into owned pouches two compartments deep, apply, save/reload the kit and hang/take the item. Counts and authored attachments must survive. An incompatible helmet/vest attachment or unsupported belt slot must be refused without replacing valid contents.
5. Repeat rack transfers and Wear on a dedicated server with two remote clients. Try simultaneous Take; only one player receives the original item. Test moving out of range, occupied return slots, death/disconnect and rack deletion during a transfer. Verify no permanent inventory lock or duplicate item.
6. Join after racks are stocked; then stream away and return. Confirm the same gear, actions and nested cargo on every client. Move/rotate/scale the rack and check that each display follows its own stand. A dedicated server must not create render-only copies.
7. Edit a roadblock from All Objects, then audio, trigger and ordinary entities. Scenario-only and unrelated Bifrost settings must stay absent. Close/reopen Properties, drag floating panels over placement/gizmo controls and release across their boundaries; no world action or invisible input catcher should remain. Check Blood text and Arsenal readability at the intended UI scale and lighting.

## Implementation decision record

REQUIREMENTS
- Finish diagnostic cleanup, replication review and identifiable GitHub publication; BI stays with the maintainer.
- Preserve the authoritative project, user assets and installed bridge; no computer use or alternate checkout.

MINIMUM COMPONENTS NEEDED
- Existing rack/native inventory and owned-controller RPC paths, with request identity and revision matching in the existing Wear service.
- Existing fixtures, release metadata and GitHub repository; one source-integrity/fingerprint script.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No replacement networking framework, release-copy project, BI upload or unearned multiplayer pass claim.

PRIMARY RISKS
- Delayed replies formerly acknowledged newer edits; fixed and covered by native assertions.
- Local acceptance does not establish transport and observer/JIP outcomes; operator checks above preserve that boundary.

REQUEST INTERPRETATION
- Publish the locally accepted fixes with quiet runtime logs and checked authority/state paths.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Release the accepted Bifrost implementation directly from this workspace with reproducible source identity and a clear BI handoff.
