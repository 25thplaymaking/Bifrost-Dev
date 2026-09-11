# Menu and vest review — 6 September 2026

REQUIREMENTS
- Keep outside-click GM lockup the highest priority; distinguish source defects from reproduced symptoms.
- Trace the layout, input ownership, native-dialog handoff and terminal cleanup.
- Verify GRS/Minnesinger mounting and the actual Arsenal discovery, selection, positioning and apply paths.
- Do not call either issue accepted until Bryce tests it successfully.

MINIMUM COMPONENTS NEEDED
- Existing property lifecycle handlers and layouts, with direct review against the released base.
- Existing catalog service, Gunsmith screen and scrollable hardpoint rail.
- Existing native regression harness and this evidence record.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No new menu framework, hard dependency on a vest mod, release or publication.
- No claim that direct method calls prove physical pointer routing or rendered layout behavior.

PRIMARY RISKS
- Consumed mouse phases, native modal ownership and deferred callbacks need actual interaction evidence.
- Native mount compatibility alone does not establish item availability in the UI.
- Duplicate identical pouches must keep both storage identity and UI selection identity.
- Local preview tests do not prove server application, remote rendering or JIP.

REQUEST INTERPRETATION
- Review the existing candidate deeply and correct any further demonstrated integration faults.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
Explain the evidence connecting the GM lockup to input-blocking layout state, correct remaining Bifrost faults, and leave acceptance explicitly pending the user's test.

## Additional source findings

- The GEAR tab excludes non-consumable clothing-area items. Building the mount picker from GEAR and WEAPONS therefore omits native clothing accessories even when their mounted placement works.
- Mouse callouts use one unbounded column: first row y=96, row height=56 and gap=10. Nineteen rows end at y=1340; seventeen end at y=1208. These exceed common available panel heights. The existing hardpoint rail already provides a bounded scroll layout and mouse wheel support.
- After moving a part, the screen follows the first matching prefab. Two identical pouches can therefore switch the selected UI row to the other pouch even though their saved storage pins remain distinct.

The next changes use the authorized catalog directly for mount candidates, use the existing scroll rail for clothing on either input device, and follow the moved part by both prefab and target slot.

## Highest priority: GM outside-click lockup (P0, acceptance open)

The comparison is against released base 37c79878b7b7d530a147a6a88f2bd512d747a3bd in the isolated candidate.

### Evidence that directly relates to the reported interaction

1. DCO_ScenarioBackdrop is a full-screen ButtonWidget behind DCO_ScenarioPanel. The authored anchors cover the entire viewport. Its decorative dim image ignores the cursor.
2. The released DCO_ScenarioBackdropHandler returned true from mouse-down, mouse-up and click, but OnClick performed no cancellation. Thus the outside-click surface consumed input without ending the property transaction.
3. The released embedded panel subscribed to attributes-start only. Native confirmation/cancellation could therefore end the manager's transaction without running a corresponding embedded-panel teardown.
4. DCO_MenuBackdrop is a second full-screen catcher, above the property panel but below its dropdown. Closing a dropdown alone should leave the property editor active; ending the property transaction must close both catchers.
5. Native property dialogs set a Bifrost flag that disables the root widget. Their deferred handoff and heartbeat participate in input ownership. The candidate prevents a queued handoff from acting after closure and cancels terminal native closure unless the transaction already ended or was deliberately handed to Bifrost.
6. Leaving GM tears down the root and restores hidden native widgets. That is consistent with Bryce's Y/leave/re-enter recovery, but is an inference rather than a captured failure trace.

The existing 1.5-second stale native-dialog recovery and controller root enable/disable behavior were already in the base. They are not new fixes from this session. Likewise, the full-screen backdrops were already authored; the new GM fix changes their behavior and cleanup, not their geometry. The GM layout diff in this candidate adds player paging, not the outside-click repair.

### Candidate closure behavior

- Outside left-click calls CancelPropertySession; other mouse buttons are consumed without cancellation.
- Confirm/cancel events clear editing state, pending category/conditional/time callbacks, option picker state and attributes.
- Cleanup hides the property panel, its backdrop, dropdown and dropdown backdrop, and preset menu; releases focus; restores suppressed overlays; re-enables the Bifrost root.
- If the attribute manager disappeared and cannot emit an end event, local cleanup still runs.
- Native dialog closure removes the deferred handoff. Completion is marked before native CloseSelf callbacks to avoid recursive cancellation.
- Deliberate native-to-Bifrost handoff preserves the active transaction.

### What is not established

No client-side trace captured the actual lockup during Bryce's click. Server logs cannot establish which client widget held focus or received that click. No physical pointer replay has demonstrated native mouse-down/up-to-click dispatch, blank-area hit testing, or absence of click-through. The same applies to native fallback dialogs, rapid reopen and gamepad focus in a live GM session. These remain P0 acceptance requirements.

## GRS and Minnesinger: identification and wiring

The prior native test loaded the installed mods and their dependencies. It used these actual resources:

| Vest | Tested part | Native result |
| --- | --- | --- |
| GRS MFCR Black, BE491CE7D9FF74CA | M67 FILBE black pouch, 22850DBB9FF3D973 | 19 root mount nodes; 10 positions accepted for this part |
| Minnesinger FCPC, A0A27FF148640F4E | Ferro Banger MC back panel, 7EC9A9DF1B770AF2 | 17 root mount nodes; 2 positions accepted by native rules for this part |

The counts describe native acceptance for those particular resources, not universal support for every vest/part combination. The second Minnesinger position has not been visually approved as a sensible attachment location.

The earlier implementation expected WeaponAttachmentsStorageComponent. These vests expose clothing/equipment storage instead. AttachmentStorage now selects weapon storage, then ClothNodeStorageComponent, then BaseEquipmentStorageComponent; MountAccepts uses the actual native slot and item-area rules. Preview, saved pins, capture and authoritative apply use that same storage identity. No GRS or Minnesinger class dependency was added.

Additional catalog verification on this review:
- GRS Essentials Configs/GRS_Nade_Pouches.conf lists 22850DBB9FF3D973 as EQUIPMENT, supply cost 0. GRS framework supplies the faction multi-list integration.
- Minnesinger Gear Configs/EntityCatalog/US/InventoryItems_EntityCatalog_US.conf lists 7EC9A9DF1B770AF2 as VEST_AND_WAIST, supply cost 1.
- Those classifications expose why the prior GEAR/WEAPONS-only candidate pool was insufficient. The new clothing-mount pool reads the active catalog source directly, retains scenario restrictions and item costs/ranks, then applies native per-slot compatibility.

The UI path is Soldier clothing row → CUSTOMIZE → ATTACHMENTS / INSPECT → Gunsmith clothing inspection → mount row → compatible part tile → Wear/apply. OPEN CONTENTS is a separate large action for stored gear. Mount-specific position adjustment is intentionally absent. This is the implemented route; it has not yet been completed interactively on a dedicated-server client.

### Additional corrections from this review

- Clothing mounts use the existing bounded, scrollable hardpoint rail for both mouse and controller. Weapon mouse inspection retains its callout chips.
- Picker height grows by 48 layout units when POSITION is visible. The rail reserves the actual candidate panel height plus bottom margin/gap, preventing the previous overlap.
- Follow-selection now matches prefab plus target storage slot, so moving one of two identical pouches cannot select the other pouch.
- Native slot pins remain authoritative inputs to validation; incompatible, occupied or invalid locations do not become arbitrary free-placement coordinates.
- Wear/apply continues through the server-side apply gate and inventory bridge. This review did not bypass station inventory, scenario item-set policy, rank or supply handling.
- Parts nested inside other mounted parts are preserved in saved data, but this change does not provide arbitrary nested-part positioning controls.

## Verification and boundaries

- Prior native inventory/saved-kit suite: 18 checks passed with installed GRS/Minnesinger. It covers native mounting, exact moves, duplicate pins, rejection, removal and serialization. It did not open the Arsenal.
- Prior property lifecycle suite: 34 checks passed; those directly invoked callbacks with null widgets.
- New native layout suite: 19 checks passed on 6 September in Workbench log session logs_2026-09-06_12-01-43. It creates both shipped layouts and verifies actual backdrop teardown/focus release, scroll/mouse handlers, 19 clothing rows, exact duplicate selection and space reservation. The subsequent revision removes the failed position control and gives OPEN CONTENTS a 280 by 58 native action. It invokes OnClick directly and uses synthetic rail entries; it is not a rendered GRS/Minnesinger interaction test.
- Source binding check: 366 checks across 26 layouts, zero failures; two fault-injection checks passed.
- Native WORKBENCH validation after the additional production changes: zero errors, 14 base-game warnings.
- A test-only missing strong reference was corrected before the successful native layout run.
- User acceptance, physical click routing, visual placement, live catalog population, dedicated-server apply, ordinary remote-client visibility and JIP remain unverified.
- No release, Workshop upload, deployment or claim of user acceptance was made.

## Required acceptance sequence

GM first: for an object, AI, player and native fallback editor, open properties, open a dropdown, click outside the dropdown, click outside the editor, immediately reopen, then select another entity. Repeat with confirm/cancel and pending category changes. Each step must leave usable input, no invisible catcher, no unrequested commit and no requirement to hold Y or leave GM. Exercise blank margins and multiple viewport sizes.

Then each tested vest: open its attachment list; reach the last mount by scrolling; select the actual mod part; add two compatible duplicate pouches where supported; remove one; open Contents and add/remove stored gear; save/reload; Wear/apply. Check that the picker and OPEN CONTENTS remain usable without overlapping the mount list. Verify resulting gear from another client and after JIP. Bryce's own successful test is the final acceptance gate.

## Test cleanup

Temporary Game and WorkbenchGame EnfusionMCP source folders were removed. Production-only WORKBENCH validation then passed with zero errors and 14 base warnings. The test Workbench process (PID 33044, candidate addon.gproj) did not exit after a normal close request. Automatic approval review rejected force-closing it with the reason 'blocked by policy'; no more specific reason was supplied. It remains open, with no child process reported. No other Workbench process was targeted.

## Clarified vest presentation: shared Gunsmith table

REQUIREMENTS
- Put the selected vest or other supported attachment-bearing item on the existing Gunsmith table.
- Use the same 3D rotation, zoom, mount selection, compatible-part picker and immediate preview updates as weapon customization.
- Keep every attachment slot reachable and preserve the server-validated Wear/apply path.
- GM outside-click lockup retains highest priority; user acceptance remains required.

MINIMUM COMPONENTS NEEDED
- Existing GRSA_GunsmithScreen, GRSA_WeaponStage and shared studio.
- Existing clothing mount data and scrollable slot controls around the 3D item.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No additional preview scene, menu or alternate editor is necessary.

PRIMARY RISKS
- Actual vest framing, attachment appearance and pointer interaction still require a rendered test.

REQUEST INTERPRETATION
- The user's intended experience is the Gunsmith workflow with the vest/item as the object on the table.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
Keep customization centered on the selected 3D item on the shared Gunsmith table, with the slot list supporting that preview.

Current-source trace: OnTabShow passes the clothing slot to the existing weapon stage. RefreshStage selects that clothing prefab and passes it through ShowWeapon and SyncAttachments. ApplyRestPose centers the complete preview bounds over the table and places their lowest point at the tabletop plus clearance. FrameStation sizes the camera and zoom to the item's bounds; FocusPoint and subject rotation remain shared with weapons. The previous scroll-list correction does not replace the table preview.

No additional production change is required to select this existing presentation route. This source verification does not establish that the rendered vest pose and controls have passed user acceptance.

## Handoff for Bryce's test

After Bryce explicitly requested saving and closing all Workbench sessions, a fresh inventory found only candidate PID 33044. Native inspection reported no world loaded, no open script file and no open asset-editor containers. Candidate changes were already saved on disk; there was no script buffer for Save All to target. A normal close request did not exit the process, so the saved candidate process was terminated under the renewed authorization. A final process inventory confirmed zero Workbench processes and zero children of that instance.

Use this candidate's addon.gproj for testing. Identified Bifrost changes are implemented; user acceptance and the previously documented unresolved runtime/external findings remain open. The clean-shutdown/resource assertion investigation is not settled by terminating the test instance.
