# Game Master input and layout lockup repair

Later Properties update: `GM_PROPERTIES_TEXT_SCOPE_2026-09-12.md` supersedes the data-only/native-fallback behavior described below. Empty and data-only object sessions now remain in Bifrost. The native menu suite now has 840 checks.

## Follow-up review decision record

REQUIREMENTS
- Review the last `Fix Bifrost UI layout lockups` task, its uncommitted implementation, comments and action paths against AGENTS.md.
- Verify native behavior with installed PAC1CLI and the configured edited Enfusion MCP; add regressions for evidenced gaps and provide physical reproduction actions.

MINIMUM COMPONENTS NEEDED
- Existing selection, context-menu, Properties and trigger handlers; existing Workbench widget regression and this document.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No new service, dependency, addon, polling loop, multiplayer protocol or unrelated asset changes.

PRIMARY RISKS
- Native box-selection bypasses ordinary selection guards and defers confirmation; interrupted right-click state can survive a blocked release.
- Native fallback must release every custom Properties surface without cancelling the native transaction.
- Direct handler tests do not establish physical mouse capture or dedicated-server acceptance.

REQUEST INTERPRETATION
- Complete a targeted follow-up review and repair confirmed omissions; preserve existing work and distinguish tool validation from live acceptance.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Make window transitions and interrupted gestures recover within the same GM session, then give repeatable acceptance steps.

REQUIREMENTS
- Investigate intermittent loss of Bifrost interaction across selection, right-clicks, Properties and other windows; repair the causes without requiring Game Master to close or resetting its camera.
- Review prior fixes and notes, Enfusion MCP contracts and installed native source through PAC1CLI.
- Preserve existing workspace changes, native server authority and separate local, dedicated-server, remote-client and join-in-progress evidence.
- Save implementation, regression coverage and evidence in `C:\Users\Bryce\Documents\My Games\ArmaReforgerWorkbench\addons\Bifrost-Dev` only.

MINIMUM COMPONENTS NEEDED
- Existing GM controller, window/session handlers and input bridges, with shared input eligibility in the controller.
- Existing Workbench regression infrastructure and this record; installed Enfusion MCP and PAC1CLI.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No UI redesign, replacement addon, new framework, deployment or publication.
- The original task used its available direct MCP connection. Persistent Codex setup was completed subsequently; see the current connection record below.

PRIMARY RISKS
- Widget events and action listeners receive the same gesture independently.
- Native dialogs and Bifrost windows have separate lifecycles; stale modal/focus state can outlive visible controls.
- A passing handler test or compiler check does not establish physical input routing or multiplayer acceptance.
- Existing uncommitted `CanRenderAttributeSession` changes must be retained while resolving its disagreement with the handoff check.

REQUEST INTERPRETATION
- Review the complete Bifrost UI ownership path and implement evidenced repairs in place, including regressions; use available native validation and report unverified acceptance honestly.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Keep Game Masters able to interact with Bifrost through window transitions without leaving GM or losing camera position.

## Original repair baseline

- Exact workspace path verified before authoring. `addon.gproj`: GUID `6A0C2D6CE9809C6E`, ID `BifrostDev`, title `Bifrost-Dev`.
- HEAD `91652a1`; pre-existing changes include scenario renderability and asset/metadata edits. These are preserved.
- No Workbench/game process was running at initial inspection. Nine `cmd.exe` processes existed; CIM process inspection was denied by the environment. No MCP registration has been edited.
- Installed Enfusion MCP 0.6.1 responds to a direct, hidden Node MCP client and shuts down after use. This is older than the custom bridge described in repository notes.
- Installed PAC1CLI responds and reads the installed game's `data007.pak`.
- Historical September 7 menu tests exercised handlers on real widgets, not physical input. Later review notes identify inconsistent input guards; subsequent commits addressed only some paths.

## Findings and repairs

1. **Input guards disagreed.** Custom Properties and visible click-catchers were not consistently included in modal ownership. Gizmo release/render and trigger-drag paths checked only native Properties; panel coverage omitted several floating surfaces. The controller now supplies the shared modal and panel checks used before context evaluation, placement, selection, gizmo interaction and trigger completion. The entity tree remains an intentional selection surface.
2. **Native context-menu cleanup was incomplete.** The bridge hid the native widget without running its close lifecycle. Native close clears workspace focus, even for a hidden menu; the installed action-menu implementation also leaves the hovered entity's UI-refresh close callback subscribed. The bridge now invokes native closure before showing Bifrost, removes that subscription on close, and restricts native focus release to the native menu's own descendants. A late close still emits the native lifecycle event without stealing focus from a newer window. Right-clicks on Bifrost are rejected before native action evaluation can change selection.
3. **Native Properties ownership depended on a timer.** A 1.5-second heartbeat timeout could re-enable Bifrost while a real native dialog remained open. Ownership now follows a weak reference to the actual dialog and its open/close state. Late closes from another dialog cannot release the current owner. The existing viewport poll recovers a destroyed owner and reconciles disabled roots; external menus retain input and Escape/Back ownership.
4. **Custom Properties handoff disagreed with rendering.** The pre-existing workspace rendering fix accepts a session containing both visible and data-only attributes, but the handoff rejected it. Both now use the same predicate. Missing manager completion also closes the panel, dropdowns and backdrop. The pre-existing rendering change is preserved.
5. **Hidden surfaces could retain input.** The existing poll reconciles orphan scenario/dropdown catchers and focus on hidden or disabled Bifrost controls. Closing a menu suppresses world actions for the remainder of the current UI tick. Focus cleanup preserves a separate window's focus. Empty menus and late clicks on hidden or disabled actions cannot retain or reuse a stale callback.
6. **Interrupted panel gestures could continue.** Drag/resize handlers previously consumed native button routing and relied on release arriving normally. They now allow native press/release handling and stop on focus loss, hide, disable, modal takeover, or an unavailable ancestor. No additional polling loop was added.

7. **Native box-selection bypassed the ordinary selection guard.** PAC1CLI shows that `DrawFrameDown`, `DrawFramePressed` and `DrawFrameUp` do not call `IsInputDisabled`; release queues `ConfirmFrame`. The follow-up guards each phase and the delayed confirmation, cancels queued confirmation, and uses native `ResetFrame` to release the selection state, frame widgets and hover ownership. The existing menu update also cancels a frame when a modal takes over and the input context stops delivering events. Normal and Ctrl selection can start again after close.
8. **Blocked right-button release skipped native state recovery.** The earlier guard returned before native `OnOpenActionsMenuUp` reset `m_bEditorIsSelectingState`. The follow-up restores eligibility on a blocked release and initializes it before each accepted press, allowing native state checks to reject the current gesture without poisoning the next one.
9. **Native fallback left a previous custom dropdown alive.** The unsupported-attribute branch hid only the main panel and backdrop. It now uses the existing complete local end cleanup to hide the dropdown/catcher, clear picker references and discard queued category/conditional work, without calling manager cancellation. Duplicate cleanup after `EndEditing` was removed so a synchronously opened replacement session is not overwritten.
10. **Trigger rendering hid an interrupted gesture without ending it.** Modal takeover or a missing group now cancels the stored gesture on the existing render callback, even without a final input event.

The changes are client input/lifecycle repairs. Authoritative gameplay dispatch and replication are unchanged. No camera-reset or GM close/reopen workaround was introduced.

## Evidence

- Reviewed `docs/CLIENT_SESSION_REPAIRS_2026-09-07.md`, `docs/REVIEW_1.0.32_UI_RPL_2026-09-07.md`, and the recent layout-fix history, including `585c758` and `5c40675`.
- Used installed PAC1CLI to read native editor/context-menu sources from `P:/SteamLibrary/steamapps/common/Arma Reforger/addons/data/data007.pak`. Used installed Enfusion MCP API documentation for widget events, focus and lifecycle contracts. Native source was inspected for behavior; no engine source files were copied into this addon.
- User-opened Workbench log `C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/logs/logs_2026-09-12_11-13-35/console.log`, line 48, records the loaded `C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/addons/Bifrost-Dev/addon.gproj` with GUID `6A0C2D6CE9809C6E`. The path and GUID were checked before running the widget probe.
- At 11:25 EDT on September 12, the expanded `DCO_MenuLayoutRegression` returned `passed: 143` and no failures. This uses actual native widgets and direct production handler calls in edit mode. It does not synthesize physical pointer input, exercise network delivery, or prove absence of every live lockup.
- Temporary test copies were removed from both script modules. A subsequent native `ReloadScripts` completed, `IsWorkbenchRunning` reported `ScriptsCompiled: true`, and production `ValidateScripts` with `WORKBENCH` returned success, zero errors and 14 warnings from installed base-game obsolete APIs.
- `Tests/verify_feature_layouts.py`: 364 binding checks across 26 layouts, zero failures, and both injected-fault checks passed.
- `Tests/verify_attribute_layouts.py`: all three test groups passed, including coverage of all 15 supported attribute layouts.
- `git diff --check` passed. Initial metadata/asset changes remain untouched.
- Creating the existing Gunsmith fixture emits `EditBoxFilterComponent used on invalid widget type`; that layout was not changed by this repair. The widget suite reports no script exception. This warning is not evidence that live input routing has been verified.

## Follow-up validation and sweep

- Reviewed the last `Fix Bifrost UI layout lockups` task (`01a09623-bfdf-7d83-97fc-644737971e27`) and the 11 modified production script files in this repair, plus their existing test and documentation changes.
- PAC1CLI read the installed native `SCR_SelectionEditorUIComponent`, `SCR_BaseContextMenuEditorUIComponent`, `SCR_ContextMenuActionsEditorUIComponent` and `EditorAttributesDialogUI` source. Enfusion MCP supplied widget/focus API contracts and live project lookup, reload and script validation. These tools provide evidence, not a formal certification.
- Before the follow-up production changes, the expanded native-widget probe reproduced nine failed assertions across four lifecycle paths, with 144 passing assertions. This established regressions in the previous repair rather than relying on compilation alone.
- At 12:18 EDT on September 12, the revised suite passed all **155 checks**, with no failures. It includes positive recovery for ordinary and Ctrl box-selection after modal closure. The suite creates native widgets and calls actual handlers in edit mode, with no active GM, no play-world creation and no global call-queue advancement.
- Both static layout suites passed again: **364 bindings across 26 layouts**, two fault-injection checks, and all three attribute test groups covering **15 layouts**.
- Both temporary test copies were removed. The edited Enfusion MCP then completed native production reload and `ValidateScripts(WORKBENCH)`: **success, zero errors, 14 warnings**, all from installed base-game obsolete APIs.
- The comment sweep removed redundant banners and comments, removed an obsolete deferred-feature note, corrected the session-lifetime description, and retained local explanations of focus ownership, event ordering, coordinate units and cleanup ordering.
- The action sweep traced world selection, context evaluation/dispatch, placement, gizmo updates, trigger completion, Properties handoff, close/reopen, and teardown. It preserved the native entity-tree selection exception and the existing rights checks and reliable server RPCs for transforms and trigger links. Server code resolves replicated targets and rejects missing or inappropriate entities; no client-authoritative gameplay path was introduced.
- Asset and metadata changes were preserved. No new dependency, service, background poll or alternate addon was created. `cmd.exe` count stayed at **15**. No test process was left running.

## Physical reproduction and acceptance actions

Use the exact workspace and `addon.gproj` identified above. Start a test scenario as GM with one AI group, a separate unit, a vehicle or prop, and a Bifrost trigger. Keep the camera on an easily recognizable landmark; record its pose if the scenario provides camera coordinates. Use the production build with temporary regression handlers removed.

After each sequence, use this recovery check: select the separate unit, right-click it, dismiss the menu, open Properties, close it, then move the camera deliberately a small amount and return. Every control must respond without exiting GM, using RESET UI, or forcing a layout rebuild. No close gesture may also place an object, change a link or manipulate a world entity.

| Sequence | Reproduction actions | Expected result |
| --- | --- | --- |
| Original lockup loop | Alternate unit, prop and group selection. Open a world context menu, choose Properties, open a value dropdown, dismiss it, close Properties, then immediately select another entity. Run 20 cycles, alternating normal and rapid clicks. | First click after each completed close works; no invisible catcher, dead controls, duplicate action or camera reset. |
| Close routes and reopening | Repeat Properties close through Apply, Cancel/Back, Escape, outside release and the close button where available. Reopen immediately. Press outside and release over a property control, then reverse the gesture. Repeat a dropdown action that opens another menu. | Only the intended close route ends the session. Release over a control preserves it; the newly opened menu remains interactive and stale callbacks do not close it. |
| Box-selection interrupted by UI | Start a world selection rectangle; move into a floating Bifrost panel and release. Repeat with Ctrl. Also start while the pointer is over a panel and move into the world. Open Properties and attempt both normal and Ctrl box-selection behind it. | UI gestures do not start/commit world selection. An interrupted frame disappears; ordinary and Ctrl box-selection work again after close. |
| Modal takeover during a gesture | Begin box-selection or a trigger-link gesture, then use Escape/Back to cancel or open the pause menu while holding the mouse. Release before closing the menu. Repeat with a modal transition available in the scenario. | No deferred selection or trigger link commits behind the modal. No rectangle or link line resumes when the modal closes. |
| Right-click interrupted mid-gesture | Hold right mouse over a world entity, move over a Bifrost panel, then release. Repeat while a placement/selection mode is active. Quick-hide a floating panel with its grip and immediately right-click a world entity. | No world action executes on the blocked release. The next valid world right-click opens the context menu without an extra recovery click. |
| Native and external window ownership | Open Pause and an available native dialog from Options > Editor Tools. Leave it open for at least five seconds, attempt clicks on Bifrost/world behind it, then close. If an installed addon exposes native-only Properties, repeat there and switch back to supported Bifrost Properties. | Only the top dialog receives input; its focus does not disappear after delayed cleanup. Closing restores Bifrost. Record native-only Properties as unexercised if no such fixture exists; Pause alone does not verify that branch. |
| Drag and resize interruption | Move and resize Orders, Options and the gizmo panel. During each gesture, Alt-Tab away, release outside the app, return, then move the pointer. Repeat with a panel quick-hide or modal takeover, and at a viewport edge. | The panel no longer follows the pointer after interruption, remains reachable, and can be dragged/resized again. OS focus and lost release behavior require this physical test; a widget `OnFocusLost` test alone does not establish it. |
| Window churn | Open and close Arsenal, mission tools, compositions, marker, tutorial, scenario Properties and entity Properties in succession. Interleave context menus and dropdowns; repeat 10 times. Include a trigger wizard: first Apply from an earlier page should reach review, then explicit confirmation should apply. | No orphan backdrop or stolen focus. The recovery check passes after every window, and trigger settings apply only through their intended confirmation. |
| Placement and gizmo isolation | Preview a prop; open a modal and click controls above the world. Close it, then place deliberately once. Start a precise move/rotate gesture and interrupt it with a modal before release; repeat a normal complete gesture afterward. | Modal clicks do not place or move anything. A deliberate placement creates one prop. Interrupted manipulation stops and the next gesture works; previously accepted preview movement is not an automatic rollback. |
| Trigger-link isolation | Ctrl-drag the AI group onto the trigger; repeat with a drop over a panel, a cancelled gesture and a removed target. After each rejection, make one valid link. | Only valid drops reach authority; cancelled gestures leave no active line or stale target, and valid linking still works. |

Run the applicable sequences separately in (1) a local/listen-host GM session, (2) a remote GM client connected to a dedicated server with another observing client, and (3) a client joining afterward. On the dedicated server, check that placement, confirmed transforms and valid trigger links are observed consistently; rejected gestures produce no gameplay changes. A joining client must receive the accepted world and link state. Losing GM rights must prevent further authoritative manipulation. These are distinct acceptance levels; the completed edit-mode suite does not establish any of them.

For an intermittent failure, capture the sequence number, exact last successful action, input buttons still held, open window, entity type, network role and timestamp. Preserve the relevant client/server logs. Do not count exit/re-entry to GM or RESET UI as a passing recovery.

**Current acceptance status:** ready for these live tests. All automated checks above passed; physical input, OS capture, listen-host, dedicated-server, remote-client and join-in-progress acceptance remain unverified. A finite passing reproduction run cannot guarantee the absence of every possible intermittent lockup.

## Current Enfusion MCP connection

The edited local server is configured once as `enfusion-mcp` in `C:/Users/Bryce/.codex/config.toml`, using `C:/Program Files/nodejs/node.exe` directly and `C:/Users/Bryce/source/repos/enfusion-mcp-bifrost/dist/index.js`. Its package is `0.15.0-bifrost.1`, it advertises 62 tools, and the installed Workbench bridge reports `0.15.0`. Startup timeout is 60 seconds and tool timeout is 120 seconds. The checked project-local configuration example is `docs/enfusion-mcp.codex.toml`; the old npm registration recipe has been removed to avoid a duplicate server.

Live project lookup resolved `C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/addons/Bifrost-Dev/`. Native reload and validation succeeded through the configured MCP during this review. The earlier task's unavailable persistent configuration and Computer Use permissions are historical limitations, not current setup instructions. The 22 installed bridge scripts remain; only the two temporary regression copies were removed. The user's Workbench remains open with production scripts loaded.
