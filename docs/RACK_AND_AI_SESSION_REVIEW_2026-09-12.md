# Rack and AI session review

REQUIREMENTS
- Correlate the September 12 server and client logs, review Bifrost first and then Bifrost-AI-Compat, and fix attributable runtime faults.
- Move CDD back panels with Hang/Take Vest, preserving original items, attachments, nested cargo and damage.
- Add a primary rifle rest to the XL cross, with Hang/Take outside the Arsenal.
- Allow a GM to name a cross, assign an operator label, and opt into restoration with the server's mission save.
- Keep authority on the server and supply distinct dedicated, remote-client, JIP and restart acceptance steps.
- No computer use, server start/restart, live package replacement or BI publication.

MINIMUM COMPONENTS NEEDED
- Extend the existing rack transfer, display and replicated state.
- Reuse the existing mission settings panel and authenticated GM request route for rack settings.
- Register rack serialization with native mission persistence; native inventory serialization retains item contents.
- Add narrow AI guards in the existing compatibility addon after the main review, with regression coverage.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No new persistence database, periodic disk writer, replacement inventory or copied third-party scripts.
- Operator assignment currently means a visible label with shared access. An access restriction was offered for clarification and is not assumed.
- A native crash without a symbolized Bifrost stack is not attributed to Bifrost solely because it was loaded.

PRIMARY RISKS
- A paired vest/panel transfer can partially fail; completed moves need rollback and all original items must remain recoverable.
- Client snapshots may precede inventory entities. Shared settings and displays must recover after JIP and streaming.
- Persistence must save the inventory under the rack, avoid duplicate item roots, and delete opted-out/deleted rack records.
- Save restoration and standalone native compilation do not prove multiplayer acceptance.

REQUEST INTERPRETATION
- Complete the reported rack additions and evidence-based bug fixes locally; leave live rollout and BI publication to the operator.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Make the gear crosses useful as named, optionally persistent kit stations while repairing the faults found in the matching multiplayer sessions.

## Evidence collected before editing

Authoritative workspace: `C:\Users\Bryce\Documents\My Games\ArmaReforgerWorkbench\addons\Bifrost-Dev`, `BifrostDev`, GUID `6A0C2D6CE9809C6E`; clean starting revision `ec3faff87b7de99b7ce9dc339dee16c938ecff70` (v1.0.34).

Client log times are EDT (UTC minus four hours). Client sessions `17-16-03`, `17-33-45` and `19-43-44` overlap server sessions `21-15-02`, `21-42-02` and `23-42-23`. Server package metadata selected Bifrost 1.0.34, updated at 21:32:53 UTC, and Bifrost-AI-Compat 1.0.0. The 21:15 server predates that update; package metadata alone does not establish its loaded build.

- The pre-update session contains rack entity creation failures. The later server logs contain no Bifrost-named script errors.
- At 23:41:25 UTC the server reports a null `weapMgr` in `SCR_AIWeaponHandling.GetCurrentMagazineComponent`, called by `SCR_AISwitchMagazine` while resolving a compartment weapon manager.
- The restored 23:42 server session reports 27 null `targetFaction` exceptions in enemy marking; the stack passes through native perception, Bifrost QRF, and the AI compatibility scheduler.
- The 17:33 client session reports 23 VON exceptions through `TF163_RadioDiagnostics` / `TF163_RadioReceiverReinitFix`, and native crashes occur during reconnect. These require ownership attribution; they do not establish a Bifrost layout fault.
- Native persistence performs automatic saves and restored the mission at 23:42:41. It also reports pre-existing duplicate building/item persistence IDs.
- The last server session saved and shut down at 23:49:38–41 UTC before this review. All remote operations in this review are read-only.
- Installed CDD Core prefab sources use `ZEL_BackPanel` independently of the vest; some contain nested storage, tools and pouches. Paired transfers must retain both entities rather than replacing them with prefab copies.

Initial and final MCP connection checks could not reach Workbench. Primary API documentation remained available through MCP, and PAC1CLI provided the installed native and CDD prefab sources. Validation uses bounded native Workbench jobs which verify the exact loaded project path and remove their temporary fixtures afterward; this is native compiler evidence, not a successful MCP live-validation response.

## Saved implementation

- Hang/Take Vest moves the original vest and separately worn CDD back panel together. Both destination slots are checked before starting; a failed second move rolls the first one back. If rollback itself cannot restore a slot, original entities remain where the native inventory left them and Take Vest can recover a panel left on the rack. No equipment prefab copies replace inventory items.
- CDD's 25 `ZEL_BackPanel` prefabs and the legacy `ZEL_DN_Backpanel_1CR.et` variant are classified without a CDD dependency. The legacy variant uses the Extra equipment area; other Extra items are unaffected. The bounded source inventory is in `Tests/Workbench/cdd-back-panel-source-review.json`.
- Back-panel display follows the vest's spine frame, with a bounds-based fallback. A late-arriving vest invalidates the panel display so it cannot remain at the fallback position after JIP. Client displays are local previews; server-held original inventory remains authoritative.
- The XL cross adds Hang/Take Primary Rifle after the existing Arsenal, Vest, Helmet and Belt actions. Its return destination is the empty primary weapon slot. Placement accounts for the weapon bounds and the stand's rotation and scale; the butt rests at the cross base while the barrel leans toward the upright.
- Double-clicking a cross in Bifrost Game Master opens Gear Cross Settings. Name, operator label and save option use the existing authenticated GM route. Settings replicate and are included in join snapshots. Assignment is a label with shared access, not an ownership lock.
- Opt-in persistence uses the existing mission persistence system, its storage collection, native entity/inventory/editor serializers, and a small rack-settings serializer. The rack remains a tracked inventory root when saving is off, preventing its contents from saving as independent world roots. Turning saving off removes the previous rack record on the next mission save. Missing/inactive persistence rejects the setting change.
- The companion addon guards restored targets without a faction/entity/leader and compartments without a usable turret weapon manager. Normal marking and reload behavior still use the native implementations.

## Verification and source release

- Final native rack job: **45 passed, zero failed**, `logs_2026-09-12_22-10-43`, exit code 0. It verifies this exact `Bifrost-Dev/addon.gproj`, physical replicated storage on both variants, inherited native persistence configuration, rule priority/serializers, paired-transfer failure transitions, settings validation, and native M16 geometry on a rotated/scaled XL cross. See `Tests/Workbench/gear-rack-features-result.json`.
- Combined native main/compatibility job: **792 passed, zero failed**, `logs_2026-09-12_22-08-50`, exit code 0, with both exact project paths verified. This compiles the new AI guards and final rack gameplay code alongside the existing control/resource integration suite.
- Static checks: **360 bindings across 25 layouts**, zero failures and two fault-injection checks; all **15 attribute layouts** covered; all **five AI compatibility contract groups** pass.
- Release integrity: **457 runtime files, 138 metadata GUIDs, 102 referenced attribute classes**, zero failures. Runtime-source SHA-256: `5c984a04971bdf78c7eb751a1bb6668c20bd5dee82258b5d6ff52739527b2c32`. Manifest and release documentation identify **1.0.35**, main GUID `6A0C2D6CE9809C6E`.
- The native jobs emit existing base-game obsolete-API warnings and Workbench UI/font/cursor resource-reference diagnostics at process exit. The fault-injection case deliberately emits one rollback warning. The final fixture uses the verified M16 resource GUID and has no missing-resource diagnostic. There are no Game/WorkbenchGame compilation errors in the final jobs.
- Native transfer tests inject completion/failure outcomes; they do not replace actual player inventory interaction. The custom save serializer is registered and compiles, but a real mission save/restart has not been executed. All CDD prefab coverage here is source review, not a claim that every model was visually tested.
- All temporary Game and Workbench test fixtures were removed. Owned Workbench processes exited; no process descendants remained. The pre-existing `cmd.exe` count remained 22.

The [GitHub source release v1.0.35](https://github.com/25thplaymaking/Bifrost-Dev/releases/tag/v1.0.35) contains the main addon. Bifrost-AI-Compat's two native AI guards are saved in its existing local workspace and are not bundled into the main addon. Its GitHub repository was not available; the operator handles its existing BI distribution path.

## Acceptance actions

Use matching main-mod and compatibility builds on all participants. Run the following in a test mission, first as a listen server and then independently on a dedicated server with two remote clients. The production server was not started, restarted or modified by this review.

1. **Rack placement and actions:** place a small Gear Cross and an XL Gear Cross from Lightning > Bifrost. Arsenal stays first. Small supports Vest and Helmet; XL also supports Belt and Primary Rifle. Opening Arsenal shows no Hang/Take buttons inside it. Recheck that gear categories and item selections open on the first click.
2. **CDD paired gear:** equip a vest and a CDD back panel with tools/pouches and a partly used magazine inside a nested container. Hang Vest once. Both leave the character, appear together on the cross, and the action becomes Take Vest. Take once and compare every attachment, container, magazine and round count. Repeat with a regular Back Panel-slot variant and the legacy `ZEL_DN_Backpanel_1CR.et` Extra-slot variant, and with stock PASGT/ALICE gear without a panel.
3. **Blocked return:** hang a vest/panel pair, then equip another back panel before selecting Take Vest. The pair must stay on the rack. Free the matching slots and retry; both return. Rapidly activate actions, and have two clients try taking the same item. There must be one original item, no duplication and no stuck busy state. Test death/disconnection during an operation and recovery by another nearby client.
4. **Rifle:** equip a primary rifle with an optic, other compatible attachments and a partly used magazine. Hang Primary Rifle on the XL cross. Confirm it leans against the stand and can be taken back with unchanged ammunition and attachments. Occupying the primary slot must block Take even if the secondary slot is empty. Repeat with a different-length rifle and a rotated/scaled XL cross. Small crosses must not offer this action.
5. **Appearance and late join:** observe both crosses from a second remote client. Move far enough away to stream the inventory out, return, then join from a fresh client after gear is already hung. All gear must resolve without another interaction; the panel must align with its vest. Check small/XL shoulder contact, helmet position and nested attachments from front and rear.
6. **Names and assignment:** as GM, double-click a cross, set `Alpha Cross` and operator `Warlord`, and apply. The world action should read `Arsenal - Alpha Cross (Warlord)`. Reopen settings, cancel changes, change labels again, and join with a new client. Everyone should see the latest labels and retain shared use. Ordinary players must not be able to alter the GM settings.
7. **Persistence at a scheduled test restart:** name and load two crosses, enable saving on one and leave the other off. Wait for or request a successful native mission save, then perform a planned test-server restart and resume that saved mission. The enabled cross should return once, with its transform, labels, vest/panel, helmet, belt, rifle and nested item state intact. The opted-out dynamically placed cross and gear should not return. Take and return the restored kit to verify its native slots. Turn saving off on a previously saved cross, save again and restart; it must not resurrect. Repeat with a GM-deleted saved cross. Starting a new mission must not import a previous session's racks. Do not restart the production server for this check without a maintenance plan.
8. **AI restore and reload:** resume the affected mission or reproduce a perceived target with missing faction/leader during restore. The server must no longer emit `SCR_AIEnemyMarkingSystem.MarkTarget` null-faction exceptions; valid enemy targets should still be marked normally. Exercise AI in passenger seats, turret reloads, and changing compartments during reload. No `SCR_AISwitchMagazine` null-manager exception should occur, and valid infantry/turret reloads must still complete. Repeat GM QRF and optional tactical settings with a remote GM and JIP observer.

Client VON exceptions through `TF163_RadioDiagnostics`/`TF163_RadioReceiverReinitFix` and unsymbolized native reconnect crashes are recorded separately; this patch does not claim to fix them. Existing duplicate persistence IDs in the mission also require a separate controlled save/restore check rather than deleting live records.
