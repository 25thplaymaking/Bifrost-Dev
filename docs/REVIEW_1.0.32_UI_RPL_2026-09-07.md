# 1.0.32 UI, replication and release review

Review date: 2026-09-07. Source review and fresh static/Workbench validation; no gameplay changes made.

## Scope and decision record

REQUIREMENTS
- Review pending changes and the last three patches, prioritizing existing-feature bugs and UI quality.
- Trace menu ownership, secondary actions, server authority, replies and stream-in/JIP behavior.
- Identify release blockers and an appropriate 1.0.32 release window.
MINIMUM COMPONENTS NEEDED
- Existing source, Git history, release records, native engine source and existing validators.
- This review record in the authoritative workspace.
REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No implementation, staging, publication, deployment or alternate checkout is part of this review.
- A fixed release date was not supplied. Scheduling recommendation is conditional on acceptance.
PRIMARY RISKS
- Widget consumption and input-action listeners have separate paths.
- Local success, reliable transport and compilation do not establish remote presentation.
- Published version numbers are not currently tied to a complete local source/package record.
REQUEST INTERPRETATION
- Review the pending working tree, rather than silently stage it; separate findings from historical fixes and test gaps.
UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Make 1.0.32 a focused stabilization release with trustworthy UI ownership and multiplayer outcomes.

## Exact review baseline

- Workspace and Workbench project lookup both resolve to `C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/addons/Bifrost-Dev/`.
- `addon.gproj`: GUID `6A0C2D6CE9809C6E`, ID `BifrostDev`, title `Bifrost-Dev`.
- Branch: `bishop/runtime-scale-all-entities`; HEAD: `8cff2a0b098f9b5c471821e498e62dbdbdc86124`.
- **Nothing is staged.** `git diff --cached --quiet` returns 0. There were 189 changed/untracked status entries before this report. Untracked directories count as single entries.
- The tracked-only diff understates the pending implementation: many GRSA files are deleted while their BIA replacements are untracked. Those replacements were included in this review. Staging tracked changes alone would omit essential source/assets.
- This is a targeted deep review of the UI dispatch and recent-feature replication paths, not proof that every action in the addon works. Older formation, defence, QRF and CQB outcomes have historical review records, but were not independently exercised here.

## Prioritized source findings

### F1 — P1: custom target selection runs before the native input-disabled check

Source: `Scripts/Game/DCO/Trigger/DCO_TriggerSyncSelection.c:13-26`.

`EditorSetSelection` tries service-access placement, Arsenal-access selection and animation selection before calling `super.EditorSetSelection`. Installed native `SCR_SelectionEditorUIComponent.EditorSetSelection` begins with `if (IsInputDisabled()) return;`. Bifrost therefore executes its side effects before the native check for disabled selection, missing managers and clicks on unrelated UI.

The override of `IsInputDisabled` at lines 5-10 does not fix this ordering. Animation/Arsenal targeting check native Properties individually; service `SelectAtCursor` has no equivalent UI guard. Custom Properties and floating panels are not covered by those native-only checks.

Reproduction target: arm Move Service Access, click a panel control while the world cursor still has a valid position, and verify that the access point does not move or consume the armed operation. Repeat with Arsenal Access and Animations FX. This is a source-confirmed missing guard; the physical pointer sequence was not reproduced this turn.

Address: check native input eligibility and custom UI ownership before dispatching any of these operations. Keep explicit entity-tree targeting working; a blanket ban on all UI widgets would break an intended selection route.

### F2 — P1: precise transform and attach do not respect custom menu ownership

Source: `Scripts/Game/DCO/GMUI/Gizmo/DCO_GMGizmo.c:355-397`, `:412-423`, `:521-525`.

The gizmo independently listens to `EditorTransform`. Its down, up and render paths only check `IsNativePropertiesOpen`. They do not check custom Properties, context menus, Mission Tools, floating panels or the widget under the pointer. If precise mode has a target and a gizmo handle is behind a panel, down can start a drag. Armed attach dispatches even before handle picking. Subsequent movement/commit uses the authoritative transform route, so server permission checks do not prevent this accidental but authorized action.

Opening native Properties cancels interaction, but handing ownership back to the custom shell clears the native flag. Custom Properties opening does not provide a persistent gizmo guard.

Address: gate start and ongoing interaction on the current owner; cancel a captured world interaction when a modal takes ownership. Verify release over a panel cannot commit an unintended transform. Physical overlap tests remain required.

### F3 — P1: the placement repair still excludes floating utility panels

Source: `Scripts/Game/DCO/GMUI/Create/DCO_GMPlacementConfirm.c:131-150`; `Scripts/Game/DCO/GMUI/DCO_GMOptionsPanel.c:465-481`.

The new guard correctly blocks custom Properties, its backdrop, the context backdrop, native Properties and a disabled/missing root. It still tests only CREATE, EDIT, top bar and context-menu rectangles. Options opens by showing its own floating widget; it does not enable one of those backdrops. Orders, chat, overlay controls and other floating surfaces likewise need an explicit ownership decision.

With a prefab selected, a click in Options outside the four guarded rectangles can still reach `CreateEntity`. The same guard is used during mission/composition world targeting. Passing the current Properties regression therefore does not prove that all menus are click locked.

Address: cover actual interactive surfaces in the existing controller/placement guard. Keep world targeting available outside nonmodal panels. Test a selected prefab plus each floating panel, including drag/resize and scroll interactions.

### F4 — P1: Wear acknowledgements can clear a newer draft or a new session

Source: `Scripts/Game/Network/BIA_ResourcePlayerControllerInventory.c:230-242`; `Scripts/Game/UI2/Shell/BIA_ShellMenu.c:338-389`; `Scripts/Game/Loadout/BIA_DraftService.c:620-675`.

Wear results contain status/counts/cost/sample, but no request ID, target ID or draft revision. The shell subscribes to a static invoker and clears the current service's `m_bDraftDirty` on SUCCESS or PARTIAL. Editing remains possible while the request is outstanding; the send debounce is not a revision lock.

Concrete failure sequence: send draft A, edit to B before A's reply, receive SUCCESS for A. B is now marked clean despite never being worn. Closing with apply-on-close enabled can consequently skip B. Closing and reopening before an old reply also lets a different shell/session accept it.

Address: correlate request, session/target and draft revision; clear dirty state only for the acknowledged revision. Keep edits after send dirty. A pending indicator and explicit failed/partial results should reflect that same request. Mission Tools and Service Bay already demonstrate request-correlated patterns that can be adapted locally.

### F5 — P2 release blocker: version and source identity are inconsistent

Sources: `Configs/Release/BifrostRelease.conf:2-3`, `README.md:27`, `CHANGELOG.md:3-5`, `docs/RELEASE_1.0.30.md`, `docs/RUNTIME_SCALE_2026-09-04.md:128`.

The embedded fingerprint names 1.0.29. README calls 1.0.30 an unpublished preparation. Local tags include 1.0.30 but not 1.0.31. A historical client record reports downloading 1.0.31. The old release instructions also point outside the authoritative workspace; those instructions conflict with the current AGENTS.md and must not be followed for this release.

Address before publication: reconcile 1.0.31's actual package/source if recoverable, explicitly record any unrecoverable boundary, and give 1.0.32 one exact source revision, package fingerprint and matching server/client build. Update the current notes in this workspace. No evidence here establishes today's live Workshop version or a scheduled release date.

## Ownership assessment

| Surface | Current owner/behavior | Remaining work |
| --- | --- | --- |
| Native Properties | Native dialog disables Bifrost root; heartbeat and close cleanup restore it | Handoff, native fallback, loss of target and repeated reopen with real input |
| Custom Properties | Scenario panel owns transaction; outside left release cancels; recent placement guard blocks its modal | F1/F2; include Defend/artillery and native compound controls |
| Context/option popup | Shared context menu at high Z order; Escape checks it first | Handler is OnClick-based, unlike repaired Properties release path. Test down/up without assumed OnClick, right click and popup-to-parent focus return; do not assume the old Properties failure automatically reproduces on this ButtonWidget |
| Mission Tools/compositions | Placement bridge explicitly blocks while open; request sequence rejects old mission replies | Other input consumers still need ownership checks; close while pending is UI dismissal, not rollback of a request already sent |
| Options/orders/chat/overlays | Separate floating widgets | F3; text entry, scroll and drag must not operate world tools |
| Arsenal/Gunsmith | Native super-menu; Back folds child content; guarded attachment tab transition | F4; keyboard Wear while a confirmation owns focus, physical attachment clicks, Exit versus Back behavior |
| Vehicle Service | Menu contexts; pending request IDs, capability acknowledgement, timeout and cancellation | Real Back/cargo/progress transitions; vehicle departure/deletion, server refusal and cancel/completion race |
| Passive hint | Disabled widget, no focus request, expiry and game-end cleanup | Verify gameplay remains uninterrupted at different display scales |

Recommended invariant: each input gesture has one owner from press through release. Modal ownership overrides world actions; a nonmodal panel blocks world actions over its interactive area; text entry suppresses unrelated shortcuts. A closing gesture is not a new world click. Put the small shared ownership decision in the existing GM controller and route existing consumers through it; a new UI framework is unnecessary.

## Replication review of recent features

| Feature | Source path and useful evidence | Unresolved acceptance / risk |
| --- | --- | --- |
| Scale | Server validates IDs/range, resolves player delegates, skips selected descendants, writes RplProp and RplSave/RplLoad; owner RPCs to connected players retry unresolved targets | Ordinary observer, simulation owner, late join and stream-out/in must see the same scale. The retry path and replicated snapshot write the same field without a common revision: test rapid changes/reset during stream-in for stale replay. This is a risk to reproduce, not a demonstrated network ordering failure |
| Movement/health/camera | Server sets movement factor and proportional max health; extra movement runs on simulation owner; cameras adapt locally | Injury ratios, AI/player ownership, low frame rate, obstacles, vehicles, reset and third-person/ADS. Bounded assistance caps speed at 35 m/s; UI multipliers are not unlimited speed guarantees |
| Visibility/invincibility | Server GM relay; replicated state; visibility retains child claims and restores after final release | Ordinary observer/JIP, teleport/actions, attach/detach, hidden parent+child, reset in either order, death/respawn. No claim that a replacement character inherits old-character state |
| Teleporters | Replicated endpoint settings; server pairing/proximity, pending-character identity, occupancy and arrival checks; native teleport plus client presentation | Delay cancellation, endpoint move/delete/relink, entering vehicles while pending, automatic re-entry, two GMs and JIP. Test each route separately: endpoint, marked target and named position |
| Audio | Server playback/sequence/settings; clients own sound handles; finite event duration and stop state | Packaged audible output for all six presets, loop/one-shot/fades/occlusion and trigger stop. Start time is local rather than a shared playback offset: late audible/JIP listeners start their own sample, so do not promise phase-synchronized audio |
| Hints/chatter/intel | Audience chosen server-side; owner-targeted messages; intel body remains server-side until authorized delivery; journal snapshot path exists | Selected player/faction correctness, uninvolved-client privacy, reconnect/JIP journal and deleting/removing clues. Old transient hints should not be replayed as persistent state |
| Vest mounts/nested contents | Native storage traversal and slot compatibility feed draft, save/capture and server application; server checks character/access and kit stream limits | Actual GRS and Minnesinger Wear, duplicate mounts, incompatible pins, nested magazines, capacity and save/reopen; remote observer/JIP. Local private-world inventory fixtures do not establish dedicated-server outcome |
| Vehicle Service | Request IDs and generations, server authorization/start acknowledgement, capability mask and reliable owner results | Cancel versus completion, range/vehicle change, death/disconnect; repair/refuel/rearm final state on observer. Better reply ownership than Arsenal Wear, but no fresh multiplayer pass here |
| Pause/GM teardown | Pause owner checks and prior-state restoration exist; cleanup can release only owned pause | Two GMs, exiting GM, disconnect and reconnect. Do not weaken authorization just to eliminate refusal logs; verify another GM's state is not resumed accidentally |
| Tracer | Cosmetic branch returns before projectile spawn; live branch launches ammunition; ephemeral cue uses unreliable broadcast | Remote cue/sound, live damage and cosmetic no-damage. Missing historical shot cues on JIP are expected; persistent settings/future shots must remain correct |

Reliable RPC delivery is not proof that its referenced entity is already streamed in, that the receiving menu is still the same session, or that local rendering retains the result. Server acceptance and an observer's visible outcome need separate evidence.

## Last three patches and release history

The last three commits at this checkout are all September 4, not three independently verified Workshop releases:

| Commit | Intent | Review implication |
| --- | --- | --- |
| `9de05e5` (v1.0.29 target) | Gunsmith activation; static assembly scaling | Improved widget activation and narrowed scaling assumptions; physical pointer acceptance was explicitly deferred |
| `23de0f8` | Expand scaling to all GM entity categories | Considerably enlarged the physics/animation/ownership acceptance matrix; scalar readback alone cannot close it |
| `8cff2a0` (HEAD) | Primary-button guards, target resolution, cosmetic tracer safety | Corrects individual dispatches, but does not unify input ownership across action listeners and custom panels |

The local v1.0.30 tag resolves to `37c7987`, with `3b70d8e` and `37c7987` beyond this HEAD adding field tools and delayed-teleport protection. Current dirty files incorporate additional later work, so HEAD alone does not identify the candidate. The release-level view is 1.0.29's broad GM/UI changes, 1.0.30's field tools/corrections, and a 1.0.31 client-version observation whose complete immutable source baseline is unavailable in the reviewed local records. Do not fabricate a 1.0.31 diff from today's working tree.

## Existing repairs to retain and verify

Current source contains the repaired Properties release handler and placement modal checks, player-list paging, source-mod search, deeper owned-storage traversal, native vest mount support, preview lighting ownership and scale stream-in retries. They should not be described as newly fixed by this review.

The previous September 7 record reports a 49-check native menu regression with shipped widgets, and a 21-check client-session/storage suite. Those are historical handler/fixture results. They do not cover F1's native override, F2's physical gizmo overlap or all of F3's floating surfaces. No native regression was reinstalled or rerun here, and no disposable world was created.

## QoL after blockers

- Show the exact target and operation while a tool is armed; Escape should cancel that owner first.
- Make Wear's pending/success/partial/failure states refer to the acknowledged draft. Preserve rejected or newer edits.
- Expose truncation in the item browser: `BIA_ItemListPanel.RebuildRows` stops after 300 matches with no full-result paging. Prefer an explicit count/narrow-search hint or bounded paging over removing the cap and spawning thousands of widgets.
- Restore focus to the parent control after a popup closes, then verify Back closes one layer at a time.
- Make naming consistent: saving a draft, wearing it, applying server settings and dismissing a panel are different actions. Changing these semantics requires deliberate agreement, not an incidental close-handler edit.

## Proposed release window and acceptance boundary

Recommendation: **hold 1.0.32 for stabilization; do not add new feature families to it.** No fixed date is supported by the evidence available in this review.

1. Correct F1-F4 together around existing components, update regression coverage, and settle release/source identity. Preserve the current workspace and all unrelated edits.
2. Run one physical UI pass on the resulting candidate: prop, AI, player, Defend and artillery Properties; dropdowns; outside release; Apply/Back/Escape; rapid reopen; target deletion; floating controls with a prefab armed; precise move/rotate/attach; mission/composition targeting; Arsenal and Service transitions. No world mutation from a menu gesture and no hold-Y recovery.
3. Run a dedicated server with a remote GM and ordinary observer; add a fresh JIP/reconnecting client. Use the same identified candidate on all peers. Combine scale+hide+movement+teleport+equipment/reset, then cover vest Wear, targeted messages, audio, service and pause ownership. Add a second GM for concurrent edits/teardown. Denied operations must leave server state intact and report failure accurately.
4. Complete a normal mission-length soak without the reported input lockups or new state divergence; package and verify the exact accepted source. A failure reopens the relevant gate rather than merely extending a timer.

The earliest defensible release window is the first slot after these checks pass. A UI-only hotfix with the multiplayer work deferred would need an explicitly agreed narrower scope; it would not satisfy this full 1.0.32 stabilization review's acceptance goals.

## Fresh validation in this review

- Exact workspace identity and live Workbench path confirmed.
- WORKBENCH validation: **0 errors, 14 base-game deprecation warnings**.
- Feature layouts: **362 bindings / 26 layouts, 0 failures; 2 injected-fault checks passed**.
- Field resources: **PASS**, including six audio source hashes, graph/platform metadata, settings and teleporter/hint syntax. No packaged playback claim.
- Engine contract inspected through MCP API, BI Multiplayer Scripting documentation and installed native selection/editable-entity source read from game archives.
- No production code changes, Git staging, gameplay simulation, server mutation, new Workbench instance or publication performed.

Reference: [BI Multiplayer Scripting](https://community.bistudio.com/wiki/Arma_Reforger:Multiplayer_Scripting). Existing evidence records used: `CLIENT_SESSION_REPAIRS_2026-09-07.md`, `BUG_REVIEW_2026-09-06.md`, `RUNTIME_SCALE_2026-09-04.md`, `RELEASE_1.0.30.md`, and `Tests/Workbench/README.md`. Historical results remain separate from this turn's checks.
