# Bifrost bug recap and review — September 6, 2026

**Acceptance: OPEN. Bryce's reproduction and confirmation are required before any reported regression is called fixed.** Compiler, source and automated regression results are evidence about the candidate, not acceptance of the reported gameplay.

## Decision record

REQUIREMENTS
- Recover every reported bug and unfinished request from the September 5 / early September 6 sessions.
- Prioritize loss of GM input after clicking outside object, player or command properties.
- Explain previous changes, their exact checkout, and their verification limits.
- Implement all demonstrated Bifrost defects recovered from the reports and log review; keep runtime acceptance open.
- Review GRS and Minnensinger vest customization and the remote September 5 18:00–21:00 Toronto play-session logs.
- Preserve dedicated-server authority and distinguish remote observers and late joiners.

MINIMUM COMPONENTS NEEDED
- Existing isolated session-fix checkout, property lifecycle, native inventory, player list, audio and preview components.
- One consolidated review record; existing compiler, layout and resource checks.
- Read-only installed-addon and bounded historical-log inspection.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No publication, new addon identity, release-version change or replacement framework.
- Native clothing support is implemented in the candidate without adding a GRS or Minnesinger dependency to Bifrost.

PRIMARY RISKS
- Outside dismissal must cancel unconfirmed edits and consume the click before it can affect world selection.
- Session-event tests do not reproduce actual mouse routing, rendering or network behavior.
- The development checkout, released package and fix candidate are different.

REQUEST INTERPRETATION
- Recap every recovered issue, implement the Bifrost corrections and deepen inspection of unattributed faults. See SESSION_ACTION_2026-09-06.md for the expanded action record.
- Outside left-click cancels unconfirmed properties. Existing explicit close/Apply behavior is preserved.
- Server inspection is read-only.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
Account for the previous work, investigate the reported failures against actual code and session logs, and leave an identifiable candidate and explicit open acceptance checks.

## Which code contains what

| Location | State |
| --- | --- |
| C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/addons/Bifrost-Dev | Older dirty development checkout; branch bishop/runtime-scale-all-entities at 8cff2a0. Does not contain the latest session patch. |
| C:/Users/Bryce/Documents/Bifrost-Releases/1.0.30 | Released base 37c7987. Previous session records confirmed GitHub publication and Bryce's BI upload. No new publication was performed today. |
| C:/Users/Bryce/Documents/Bifrost-Fixes/session-2026-09-05 | Current candidate; branch bishop/fix-session-replication-input, based on 37c7987 with uncommitted corrections. All current corrections are here. |

All three refer to the same addon identity: **6A0C2D6CE9809C6E / BifrostDev**. Use the candidate's **addon.gproj** for testing these changes. Today's Workbench project lookup and startup log explicitly resolved that path and GUID. These patches have not been put into the published 1.0.30 package.

Recovered history: **Fix Bifrost scaling and features** (01a072b3-cd62-79e1-9af9-e55bb77f4dda), and **Review Bifrost release readiness** (01a07392-d6ac-7610-b266-ff50f3ee523b), including its final early-September-6 turns. The app summary omitted those final messages; the saved session supplied them.

## 1. Priority: properties / command menus lock GM input

**Report:** clicking away while editing objects, players, Defend or artillery leaves Bifrost unusable; holding Y, leaving GM and returning restores it.

**Previous defects found:** the custom panel subscribed to native attribute-start events but not confirm/cancel. Deferred native-dialog handoff could outlive the dialog. That allowed stale editing state, pending refreshes or focus ownership to survive a transaction's end.

**Last night's candidate changes:**
- DCO_GMScenarioPanel.c subscribes to confirm/cancel as well as start.
- OnAttributesEnded clears the open/editing flags, attributes, option-picker references, pending row/category/time callbacks, panel, background and popup visibility, overlay suppression and native-properties focus flag.
- Shutdown unregisters all three native invokers and pending conditional refresh.
- DCO_AttributesDialogPeel.c tracks whether its native dialog is still open, removes a deferred handoff on close and prevents a closed dialog's update from taking input again.
- Native manager cleanup remains authoritative. The end-event handler does not recursively confirm or cancel.

**What that did not cover:** its 25-check regression invoked session events without visible widgets. It did not test clicking the full-screen background. The background handler still consumed down, up and click without closing anything.

**September 6 additional changes:**
- DCO_ScenarioBackdropHandler now owns a reference to its property panel and sends an outside left click to CancelPropertySession.
- Press/release/click remain consumed, so closing does not intentionally forward that click into selection/placement underneath.
- CancelPropertySession also invokes local cleanup if an open modal or queued refresh survives without a manager end event.
- Other mouse buttons remain consumed without cancelling.
- A native dialog closed without Confirm/Cancel now cancels its attribute transaction. Deliberate native-to-Bifrost handoff is exempt. Completion is recorded before native close callbacks to prevent recursive cancellation.
- Pending edits use cancellation, so an outside click does not confirm an unfinished trigger wizard.

**Verification:** fresh native compile passed; the expanded regression passed **34 checks**. Nine added checks exercise the production handler's down/up/click methods, no-end-event cleanup, immediate reopen, repeated close and an absent handler owner. These are direct event-method calls with null widgets, not synthetic pointer input. No screenshot or actual in-game mouse reproduction is claimed.

**Still open:** the exact screen-lock sequence, native fallback dialog dismissal, repeated edits across object/player/Defend/artillery targets, target deletion, and the complete remote-GM interaction. The absent outside-close action is source-confirmed; it is not proof that every reported lockup has the same cause.

## 2. Scale visible to the GM but not other players

**Report:** characters were the tested targets; the editing GM saw size changes, other players did not.

The evening client log records Scale 3 accepted at **19:09:02.817**, then Scale 1 at **19:09:46.539**. These messages establish accepted requests, not observers' rendered size.

Last night retained the existing unconditional replicated scale field and added FRAME alongside POSTFRAME presentation updates. Each peer reapplies size after native animation/movement replaces a transform. Only event bits acquired by this feature are cleared. Scaling and invisibility share event ownership, so resetting one does not disable the other.

Source: Scripts/Game/DCO/GMUI/Tools/DCO_GMMissionEntityState.c.

**Status:** candidate lifecycle correction, compiled; remote rendered outcome and stream-in/JIP remain open. No owner-only replication condition was found. The precise cause of the reported remote rendering failure has not been demonstrated.

## 3. Invisibility disappears after teleporting or actions

**Report:** hidden characters become visible again, not an invincibility/damage issue.

The original Toggle Visibility cleared flags on the GM's client and kept a local array. It had no authoritative request or persistent replicated hidden state. The earlier statement that Bifrost had no character visibility control was wrong.

Last night's patch routes the toggle through the existing reliable GM-authorized server relay, changes a replicated hidden property and reapplies local visibility on recipients. Attached equipment is claimed as it appears; detached equipment releases its claim. Overlapping hidden parents/children share the original visibility until the final claim is released. Showing the target restores original visual flags. Collision, traceability and damage are not disabled as a side effect.

Sources: DCO_GMTools.c, DCO_GMToolsRelay.c, DCO_GMMissionEntityState.c.

**Status:** source defect corrected in candidate; ordinary observer, action/teleport retention, equipment changes, target deletion and JIP require testing. A replacement character after respawn is a different target. Separately spawned effects, projectiles and sound are not promised invisible.

## 4. Teleport ownership and freely editable endpoints

Earlier work replaced the attached-object setup flow with a standalone placeable endpoint. Settings include link, title, manual/automatic use, delay and radius. Points can be moved, deleted and reopened through normal GM controls; the old Remove Teleporter operation was removed.

The subsequent reviews corrected:
- Dedicated-server travel's dependence on a local GM editor.
- Delayed arrivals targeting a replaced character or one that boarded a vehicle while delivery was pending.
- Reopening the endpoint editor returning an obsolete activation-mode value.
- Last night's marked-unit teleport path directly changing a character on the server and calling Update; it now uses native editable.SetTransform for simulation-owner handling.

The reported missing catalogue entry was found under **CREATE → Effects → Bifrost → Teleporter**; no missing-entry correction was established.

**Prior user evidence:** Bryce confirmed movement and teleporters worked locally. **Still open:** dedicated-server/client ownership, delayed cancellation/deletion/relinking, occupied vehicles and late joiners. Local confirmation does not settle the later visibility/scale reports.

## 5. Scale speed, proportional health and third-person camera

Earlier work added an independent movement multiplier, linear maximum-health scaling preserving injury percentage, and scaled camera distance/collision calculations with normalized rotations.

A later API review found native OverrideMaxSpeed cannot increase speed above its normal maximum. The implementation therefore added bounded, collision-swept extra ground movement on the character's simulation owner, using server-replicated settings, excluding airborne/vehicle/ragdoll/special movement and capped at 35 m/s.

**Evidence:** Bryce confirmed local movement improvement. Source and compiler checks cover the health/camera implementation. **Still open:** remote prediction/collision, AI speed, injured characters, third-person/ADS/walls and vehicle transitions at small/large scales. No new runtime acceptance was obtained today.

## 6. Object settings using the base-game layout

Earlier work embeds supported attributes and native compound controls inside Bifrost's property shell. Unsupported attributes retain the native fallback. Teleporter settings use the endpoint editor.

Today's priority menu work concerns the lifecycle of that same shell.

**Still open:** complete editable-attribute parity and all Apply/close/outside-click/reopen routes. A correctly rendered layout alone does not prove input recovery.

## 7. Global hints

Earlier work added a passive, unfocused card with title/message and 1–300 second duration. The GM can choose all players, selected players or selected units' factions; the server validates recipients and sends reliable owner-targeted delivery.

The old local probe checked dimensions, text and teardown. **Still open:** exact recipient selection on separate clients, long text/display scaling, expiry, and uninterrupted gameplay input.

## 8. Empty audio feature and subsequent playback bugs

Earlier work supplied six recordings: crowd, conversation, dog, shouting, rifle battle and a single gunshot. Controls cover playing, volume, radius, fades, loop/one-shot, wall filtering and native reverb. Recordings were acquired as CC0 sources; they are not extracted Arma 3 files.

Later reviews changed the banks to finite playback so one-shots cannot begin another recording during fade-out, and connected trigger start/stop/status to the replicated audio state.

**New contrary runtime evidence:** the client console reports **Audio Graph Build Sounds/Bifrost/Ambience.acp failed** at **19:18:37.269**. The installed graph and signal source match the candidate after line-ending normalization; the checked rifle WAV is byte-identical. An old/different graph is not demonstrated as the explanation.

The earlier 12 resource builds and six successful playback handles were local Workbench evidence. They do not overrule this packaged-session failure.

**Candidate correction:** the graph referenced exterior-reverb bus 195591, which does not exist in the current game's Sounds/FinalMix.afm. The actual exterior bus is 197639; both graph references are corrected. The verifier can now check output IDs against the extracted current mixer. A fully started local game passed all 25 field checks, including six valid native playback handles. Packaged-session playback and audible output still require acceptance. Verify the actual candidate package and resource dependencies, all presets, trigger stop, loop/one-shot, fades, wall transitions and late joiners. Do not mark audio fixed because file/hash checks pass.

## 9. GRS / Minnensinger vest mounting positions

This was the final request in the previous session, awaiting an example. Bryce now identified GRS and Minnensinger.

Installed-source findings:
- GRS ArmorVestStorage derives from ClothNodeStorageComponent. The MFCR base defines actual clothing slots such as Front_Option_1, Dangler, MAP, Shears, PTT, side pouches and Knife, plus nested GRS equipment storage.
- MinnesingerCore defines Minne_PocketStorageComponent using equipment storage and specific clothing areas for placards, radio, pouches, side armour and back panels.
- Bifrost's GRSA_CalloutLayer.Build, preview attachment walker and ApplyAttachmentsArray require WeaponAttachmentsStorageComponent. Clothing draft arrays exist, but that does not supply a clothing-slot picker, compatible preview and full apply path.

**Candidate implementation:** a shared native-mount lookup now supports weapon, clothing and equipment storage. The slot browser, callouts, preview, draft defaults, capture, saved-kit reader and existing server apply path use it. Native slot restrictions still determine compatibility; permitted inventory changes still run through the server manager and arsenal gate. Top-level pins preserve duplicate parts at distinct authored mounts; incompatible pins are reported rather than silently moved elsewhere, and invalid/duplicate pin IDs reject the attachment set before it is altered. The unstable mount-position slider and remount callback were removed after the live log showed one move expand compatibility to all 3,095 catalog resources and immediately recurse through the slider invoker. The enlarged contents action remains available throughout clothing inspection and relies on the existing draft/container validation when an item is added, avoiding false negatives from third-party preview storage metadata. Nested attachments retain the existing automatic child-slot placement model. No third-party implementation was copied.

**Local evidence so far:** both installed mods and their dependency chain loaded with the candidate, with no permanent addon dependency changes. The final native probe detected 19 GRS MFCR mounts and 17 Minnesinger FCPC mounts, and passed 18 mount/move/duplicate/removal/incompatible-slot and saved-kit checks. The saved reader chooses the same primary mount storage as the live lookup, preventing secondary equipment storage from contributing conflicting slot IDs. The rendered position experiment failed and is no longer shipped; add/replace/remove still use native authored mounts. Native inventory checks completed while a later startup permission dialog held rendered gameplay.

Acceptance must include both GRS and Minnensinger examples: mount, move, remove, save/reopen, apply, observer visibility, reconnect/JIP and server rejection of invalid slots/items.

## 10. Previous startup delay and release metadata

Bryce explicitly described the earlier startup UI/movement delay as a fluke. That historical observation must not be used to close this newly emphasized click-away lockup.

The 1.0.30 publication record verified 187 packaged scripts but identified an embedded 1.0.29 fingerprint discrepancy. Script parity did not establish full package metadata/resource parity. Today's candidate has not been packaged or published and has no new release claim.

## Actual evening server-log review

Host: **grain.silo**, reached through its configured SSH host.
Server: **reforger-08a3268f-f6ce-4c0b-a6ed-3f8bbbea9c1a**.
Requested interval: **2026-09-05 18:00–21:00 America/Toronto = 22:00Z–01:00Z**.

The normal retained Reforger folders start at 03:49Z, after the requested period. The persistent Docker JSON log still contains the requested interval. Its total size was about 13.9 GB; only the requested 20,557,510-byte interval was read using timestamp seeks. Both boundaries were checked against adjacent timestamps.

Evidence location:
- /var/lib/docker/containers/320452592f310d8c61e1c7c7c35d10f94ad93bcd2f0a0797530501988eac1c63/320452592f310d8c61e1c7c7c35d10f94ad93bcd2f0a0797530501988eac1c63-json.log
- Byte interval [13815373297, 13835930807).
- **82,371 records**, first 22:00:09.877Z, last 00:59:58.992Z. Complete requested interval coverage within this retained container log.
- The initial Docker streaming query did not finish promptly; its owned reader was stopped and the timestamp-bounded file read supplied the evidence. No server restart, configuration or data mutation was performed.

Findings, times below in Toronto:
- **19:26:** Defend/defensive-hold state changes executed on the server.
- **19:41, 20:30–20:43:** Defend and artillery waypoint radius changes reached server code. This proves some edits were applied, not that the GM retained input afterward.
- **57 resume refusals and 57 detach-all refusals**, across GM transitions. The teardown path requests these operations; the server checks pause ownership/open-GM rights. These records do not establish a frozen client or justify weakening authorization.
- **15,324** unregistered-item RPC errors from SCR_AmbientPoleSparksComponent.
- **585** RCON client-list-full errors.
- **20:52:51:** one script VM exception, **No suppression volume provided**, in the native AI suppression behavior. The shown stack does not implicate Bifrost's property UI. Server frames continue immediately afterward.
- Numerous third-party/base resource and support-station diagnostics. These are tracked as separate issues; no causal claim connects them to the menu lock.

The server cannot record a local UI focus state it was never instrumented to observe. No recovered stack proves the precise menu-lock root cause.

## Matching client-session evidence

Files:
- C:/Users/Bryce/Documents/My Games/ArmaReforger/logs/logs_2026-09-05_18-19-03/script.log and console.log
- C:/Users/Bryce/Documents/My Games/ArmaReforger/logs/logs_2026-09-05_19-20-46/script.log and console.log

Within the requested interval:
- **287** Bacon GunBuilder preview exceptions: Entity.SetAngles has NAN angles, starting 18:45:52.077; stack GunBuilderUI_PreviewUIComponent.c:515 / M4Test_GunBuilder.c:334. This is a separate preview implementation, not a Bifrost property-menu stack.
- **57** RHS radio-source null-owner exceptions across the two sessions.
- **3** native/ACE radio-menu null-transceiver exceptions at 20:05:12.958.
- Bifrost scale acceptance at 19:09 and audio-graph failure at 19:18 described above.
- Repeated **players but 5 player rows — extras not listed** warnings. Source confirms PLAYER_ROWS = 5; higher populations are not fully represented in that list. The candidate now pages the authored five rows, sorts player IDs, clamps the current page when population changes and enables/disables previous/next controls at the boundaries. Six-player rendered behavior remains untested.
- Five GRSA preview studio-light warnings between 18:32 and 19:54. World-environment entities need not be transform children, so traversing the environment rig cannot reliably find those lights. The candidate spawns and retains three adjustable lights directly in its private preview world. Native creation, world ownership and release checks passed; visual lighting quality remains open.

**Additional inspection and action:** audio, player paging, lighting and teardown corrections are implemented in the candidate. The external faults and their exact attribution are recorded in SESSION_ACTION_2026-09-06.md; none is represented as repaired by this patch.

## Current validation and acceptance

See [the action record](SESSION_ACTION_2026-09-06.md) for the current verification ledger, source attribution, shutdown issue and final cleanup status. Earlier results remain historical evidence rather than a claim about the latest candidate.

Recommended acceptance sequence on the **same candidate build** for server and clients:
1. Edit a prop, AI, player, Defend waypoint and artillery waypoint. Change a value, click outside, and immediately use camera/selection/CREATE; reopen properties. Repeat after an option popup, Apply, Escape, native fallback, rapid reopen and deletion by another GM. No hold-Y exit/re-entry should be needed.
2. With at least six players, use every player-list page, open each player's menu, and repeat while someone disconnects.
3. Combine hidden + scaled characters, movement/actions, each teleport route, equipment changes and reset in both orders; observe from an ordinary client and a late joiner.
4. With GRS and Minnesinger, mount duplicate pouches, move one, remove one, save/reopen/apply, and compare the wearer's and observer's positions. Repeat after reconnect/JIP.
5. Check health/camera, standalone endpoints, selected/global pause, targeted hints, all six audio presets and trigger transitions, and preview lighting.

Record Bryce's result per issue and exact build. **All reported issues remain open for user acceptance.** Source corrections and automated tests do not establish that the original play-session symptoms are gone.
