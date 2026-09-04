# Vehicle Service Current Handoff

Updated: 2026-09-02

## Active objective

- Stabilize the GRS-derived Vehicle Service Bay one bounded issue at a time.
- The current change makes the authored vehicle anchor a true floor-contact marker for differently sized vehicles.
- Do not use subagents for this workflow unless Bryce explicitly reverses that instruction.

## Placement contract

- `GRSA_VehiclePreviewAnchor.et` supplies the vehicle's world-space horizontal center, floor height, and initial yaw.
- The preview vehicle is rotated first, then its complete world bounds are measured.
- One offset centers the rotated bounds over the anchor and places the lowest bound at the anchor floor plus the existing 1.5 cm clearance.
- There are no per-vehicle height values and no physics simulation in the private preview world.

## Current implementation files

- `Scripts/Game/DCO/Arsenal/Vehicle/DCO_VehiclePreviewStage.c`
- `Prefabs/UI/GRSA_VehiclePreviewAnchor.et`
- `Prefabs/UI/GRSA_VehicleStageEnvironment.et`
- `docs/VEHICLE_SERVICE_DECISION_RECORD_2026-08-31.md`

## Evidence and validation boundary

- BI API evidence confirms `GetWorldTransform` provides a world matrix; `Math3D.MatrixToAngles` is the project-native conversion used elsewhere.
- PAC1CLI review of Reforger's preview scripts confirms native preview code distinguishes local child transforms from world transforms and uses bounds for composed preview geometry.
- Workbench `WORKBENCH` script validation passed on 2026-09-02 with zero errors and the existing 14 warnings.
- Workbench compile status passed with zero script errors; source log: `logs_2026-09-02_00-23-10/script.log`.
- Runtime vehicle contact, camera framing, repeated entry, and modded vehicle variation still require Bryce's in-Workbench test.

## 2026-09-02 access-crash regression

- Reproduction ended at 01:16:52 with native heap corruption `0xc0000374` while Service Bay was constructing.
- The log spawned the first tutorial mechanic, reported its slot-prefab hierarchy faults, created shared item rows, and crashed before a normal rendered Service Bay frame.
- Workshop actor startup no longer calls `Update`, `Prepare`, or `Play` during menu construction. The prefabs' authored `AnimationPlayerComponent` setup is retained and playback begins from the existing first normal stage tick.
- The shared item row no longer contains the two unsupported `Blend mode` properties reported immediately before the crash.
- The anchor remains a floor marker. Because it is a direct child of the identity environment root, its authored local transform is used while complete rotated vehicle bounds still provide automatic vertical placement.
- Post-fix Workbench `WORKBENCH` validation and compile status both passed with zero script errors and the existing 14 warnings; source log: `logs_2026-09-02_01-26-49/script.log`.

## Next runtime test

1. Place the Service zone and an LAV, then open Vehicle Service once.
2. Exit normally and open Vehicle Service a second time in the same session.
3. Confirm both workshop actors render and animate without a crash or T-pose.
4. Confirm the LAV remains centered with its tires on the authored anchor floor.
5. If both entries pass, repeat with one tracked or differently sized vehicle without moving the anchor.

## 2026-09-02 repeated access crash

- The `01-29-31` session repeated native heap corruption `0xc0000374` at the same address after the first patch.
- The malformed blend-property diagnostics were gone, but every shared item row still emitted `EditBoxFilterComponent used on invalid widget type` immediately before the crash.
- The row contained a complete hidden `WLib_EditBox` prefab even on Service rows that cannot edit quantities; this is being replaced with one native edit box used only when direct quantity entry is enabled.
- Environment decorations are being separated from synchronous menu construction and begin on the stage's first normal tick.
- Post-fix `WORKBENCH` script validation passed with zero errors and the existing 14 warnings.
- All eight custom vehicle-system `.edds` files passed Workbench's native texture validator, so they remain enabled.
- BI API evidence confirms native `EditBoxWidget` supplies `GetText`, `SetText`, and change events; PAC1CLI confirmed the comparison refuel UI texture is a packaged runtime `.edds` in `data010.pak`.

## 2026-09-02 quantity editor VME

- Runtime reached the simplified row but raised `GRSA_ItemRowComponent.FindParentMenu`: null `m_wRoot` from `SCR_ButtonBaseComponent.OnUpdate`.
- Cause: the row's button component had also been attached directly to its child `EditBoxWidget`, invoking the button base lifecycle for the wrong widget.
- The edit box now uses a dedicated `ScriptedWidgetEventHandler`; the row component remains attached only to the row root.
- Post-fix `WORKBENCH` validation passed with zero script errors and the existing 14 warnings.

## 2026-09-02 access crash after quantity-handler fix

- The `01-30-31` session successfully reloaded the quantity-handler correction before the final access attempt.
- Service access then created sixteen `GRSA_ItemRow.layout` instances, and every instance reported `EditBoxFilterComponent used on invalid widget type`.
- The native quantity edit box does not require that filter: committed text is converted to an integer and the existing request path clamps and validates the authoritative value. The invalid filter has been removed while typed quantities remain enabled.
- The first mechanic spawned ten milliseconds before heap corruption was detected. PAC1CLI inspection of the packaged `Mechanic.et` confirms it also slots a generator and pallet that produce the two hierarchy errors in the log. That prefab remains unchanged for this isolated test because it rendered in earlier sessions and native heap corruption can be detected on a later allocation than the corrupting call.
- Fresh Workbench compile status passed with zero script errors and the existing 14 warnings; source log: `logs_2026-09-02_02-00-03/script.log`.
- Static layout checks pass with balanced braces, no remaining `EditBoxFilterComponent` in `GRSA_ItemRow.layout`, and no whitespace errors.

## 2026-09-02 vehicle orbit and actor animation

- The vehicle stage had explicitly enabled translation-track mode, which converts ordinary drag into front-facing pan and bypasses the connected GRS subject-spin callback. Vehicle drag now uses the existing Gunsmith-style subject spin; the separate pan binding and wheel/stick zoom remain available.
- Home and zoom distances now scale from complete vehicle bounds. Large vehicles are no longer forced against the old 4.25 metre home-distance ceiling.
- Both packaged tutorial actors configure an animation but do not configure looping. They were only receiving `Play()`, so their one-shot clip ended in the bind pose. Each actor now reads its authored clip, prepares it with looping enabled on the normal tick after spawn, and then starts playback.
- MCP API evidence confirms `AnimationPlayerComponent.Prepare` is the supported way to change loop setup. PAC1CLI inspection of `Character_Base_Preview.et` confirms the native preview actor contract uses `Loop 1` and `AutoPlay 1`.
- The two edit-box properties reported as unknown by the latest rendered session were removed from the native quantity editor; its value is populated through `SetText` at runtime.
- Fresh Workbench reload and compile passed with zero script errors and the existing 14 base-game obsolescence warnings; authoritative source log: `logs_2026-09-02_01-59-24/script.log`, compile completed at 10:07:25.
- MCP script/reference/prefab validation reported no new source or prefab failure. The eight warnings are pre-existing API-index limitations elsewhere in the addon.
- Runtime acceptance is now one focused pass: confirm whole-vehicle initial framing, drag through all four horizontal sides, wheel zoom/reset, and both actors continuing beyond one complete clip cycle.

## 2026-09-02 mechanic T-pose root fix

- The latest screenshot proved the looping `AnimationPlayerComponent` adjustment did not solve the mechanic bind pose. That player was a secondary clip component on a full gameplay `SCR_ChimeraCharacter`; the private Service Bay world was not advancing the character's narrative animation graph.
- The raw gameplay-character spawn path has been removed. Workshop actors now use the same engine preview-clone pipeline as the working GRS soldier stage: item-preview resolution, `CreatePreviewEntity`, `PreviewAnimationComponent`, and an explicit frame step on every visible Service Bay tick.
- The packaged narrative graph and instance are installed on each preview clone. The authored mechanic marker selects native narrative state `30`; the radio-commander marker selects state `31`.
- Actor marker transforms and visual character prefabs remain authored in `GRSA_VehicleStageEnvironment.et`. No gameplay AI, behaviour tree, replacement model, or custom animation asset was introduced.
- Fresh Workbench `WORKBENCH` validation passed with zero script errors and the existing 14 base-game obsolescence warnings. The compiled game CRC is `5521a2b8`; source log: `logs_2026-09-02_10-25-10/script.log`.

### Focused runtime acceptance

1. Reload scripts or restart the current play session so the newly compiled graph path is active.
2. Open Vehicle Service and observe the mechanic at the generator for at least one full sequence.
3. Confirm the mechanic leaves the bind pose immediately and continues the packaged mechanic sequence without freezing.
4. Confirm the radio commander also animates through his separate packaged state.
5. Exit and open Vehicle Service again once to verify actor teardown and recreation.

## 2026-09-02 progress fill and service audio

- The percentage label was correct, but `ProgressBarWidget` retained its native 0–100 range while receiving normalized 0–1 values. The modal now explicitly initializes the bar to a 0–1 range, so its fill uses the same value as the percentage label.
- `SOUND_VEHICLE_REPAIR_FULL` was not a callable ACP event. PAC1CLI showed it is an internal shader under the completion event. Repair now invokes the valid packaged `SOUND_VEHICLE_REPAIR_PARTIAL` event containing the native repair-tool samples.
- Repair, refuel, and rearm use one active audio handle. Each phase replays its packaged one-shot only after the prior clip finishes; a phase change stops the old handle before starting the next sound. A failed handle is throttled instead of retried every frame.
- Cancellation, menu teardown, completion, and authority timeout still stop the active sound and clear progress state.
- Running Workbench compilation passed at 10:38:43 with zero script errors and the existing 14 base-game obsolescence warnings; Game CRC `b7cc1cd8`.

### Focused runtime acceptance

1. Start Repair and confirm the orange fill reaches the same midpoint as the `50%` label.
2. Leave Repair active for its complete 25 seconds and confirm wrench/tool sounds recur without stacking.
3. Start Full Service and confirm only repair audio plays first, then refuel, then rearm.
4. Cancel once with Escape and confirm the sound stops immediately and the menu exits cleanly.

## 2026-09-02 mechanic narrative startup and wrench

- The latest screenshot proved the preview clone itself is healthy: the actor left bind pose but stayed in `Character_Base_Preview`'s standing idle.
- Native `SCR_NarrativeComponent` does not issue `CMD_Narrative` during graph initialization. It deliberately waits 1000 ms. The Service Bay now mirrors that ordering by stepping the graph normally for one second, then binding and issuing the marker's narrative state.
- The preview clone intentionally omits the tutorial prefab's unrelated slot hierarchy. The mechanic's exact packaged wrench is now authored on the existing actor marker and restored as a physics-disabled child of the preview skeleton's native `RightHandProp` pivot using the prefab's original offset and angle.
- No gameplay AI, behaviour tree, replacement animation, or custom prop was introduced.
- MCP API/BI Wiki review and PAC1CLI base-data inspection support the implementation. Static prefab validation passed, and Workbench compiled the current source with zero errors at 10:57:19; Game CRC `62f5d8d1`, source log `logs_2026-09-02_10-57-10/script.log`.

### Focused runtime acceptance

1. Restart the play session so Game CRC `62f5d8d1` is active, then open Vehicle Service.
2. Watch the mechanic for at least ten seconds. A neutral pose during the first second is expected; he must then enter the packaged mechanic sequence.
3. Confirm the native repair wrench is visible in his right hand and tracks the hand through the sequence.
4. Confirm the radio commander enters narrative state `31` on the same delayed path.
5. Exit and reopen Vehicle Service once, confirming both actors and the wrench are recreated without a crash.
# 2026-09-02 release-scope reduction

- Removed the Armaments tab, direct quantity entry, shared-capacity state, and compatible ammunition type/feed selection.
- Removed the server verbs and implementations that replaced mounted weapons, changed ammunition feeds or types, or applied arbitrary ammunition counts.
- The mode strip now contains exactly `REPAIR` and `CARGO`.
- Repair-tab Rearm traverses the vehicle's existing mounted weapon slots on the server and tops up each existing magazine to `GetMaxAmmoCount()`. Rocket pods use their existing default rocket prefab and native reload-barrel path. No mounted weapon entity is replaced or deleted.
- Repair, Refuel, Rearm, Full Service, cargo addition, and cargo removal retain the existing server target, distance, stationary-vehicle, and access validation.

### Focused runtime acceptance

1. Open Vehicle Service and confirm only `REPAIR` and `CARGO` tabs are present.
2. Damage a vehicle, drain fuel, and expend ammunition. Run Repair, Refuel, Rearm, and Full Service separately.
3. After each operation, close the menu, leave the service area, and confirm the same vehicle can still be opened, entered, driven, aimed, and fired.
4. Confirm Rearm restores each already-mounted authored magazine and rocket barrel without changing the weapon prefab or mount.
5. Repeat from a remote client on a dedicated server and after JIP.
6. Add and remove Cargo items and confirm the existing server-authoritative cargo flow remains intact.

# 2026-09-03 capability and damaged-re-entry repair

- The damaged-vehicle crash was isolated to private preview construction before any service request: Service spawned the full gameplay vehicle prefab, including damage, physics, fuel, weapon, and replication systems, into the preview world.
- Service now uses the same `SCR_PrefabPreviewEntity.SpawnPreviewFromPrefab` render-only path as the packed vanilla catalog. Damage callouts snapshot local points from the live vehicle and do not retain live hit-zone objects in the preview.
- Repair, Refuel, and Rearm capabilities are detected independently. The footer omits unsupported operations, while Full Service sums and displays only supported stages.
- The server recomputes capabilities, authorizes an exact duration, sends that start over a reliable owner RPC, and applies only the authorized capability set before sending the reliable result.

### Focused runtime acceptance

1. Damage a non-destroyed vehicle, open Service, close it, and reopen it at least three times; confirm no native heap fault and no preview-world gameplay entity errors.
2. Complete Repair, Refuel, Rearm, and Full Service on an armed vehicle; after every exit, enter and drive the same vehicle and operate each mounted weapon.
3. Open an unarmed repairable/refuelable vehicle. Confirm Rearm is absent, Repair and Refuel remain, and Full Service lasts 45 seconds rather than 60 seconds.
4. Exercise repair-only, fuel-only, armament-only, and no-supported-capability prefabs. Confirm the footer and Full Service duration exactly match each capability set.
5. Repeat on a dedicated server from a remote client. Confirm progress begins only after server authorization, cancellation prevents mutation, results reach the requesting owner, and all vehicle state is identical for the remote client and a later JIP client.

## Preview-bound correction after first retest

- The first render-only preview revision incorrectly passed the preview entity to `SCR_Global.GetWorldBoundsWithChildren`. The helper returned sentinel bounds, and the resulting floor offset attempted to move the preview to approximately `-3.4e38` on Y.
- Placement now reads `SCR_BasePreviewEntity.GetPreviewBounds`, validates every local bound before use, converts the local center/floor through the preview transform, and defers camera framing if valid bounds are not yet available.
- Both the selected vehicle and the decorative workshop vehicle remain render-only preview entities.

## 2026-09-03 second damaged-entry crash

REQUIREMENTS
- Opening Vehicle Service for a damaged, non-destroyed vehicle must not crash the game or Workbench.
- Preserve the server-authoritative Repair, Refuel, Rearm, Full Service, and Cargo flows.
- Preserve the textual damage and capability information needed to operate the Service Bay.

MINIMUM COMPONENTS NEEDED
- The existing prefab-only vehicle preview renderer.
- The already proven GRS Arsenal preview environment.
- The existing textual damage list and service controls.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- The custom workshop environment is removed from the live preview path because it recursively instantiates gameplay prefabs and invalid replicated lamp hierarchies in a private UI world.
- Graphical hit-zone callouts are disabled because they require live damaged-vehicle collider traversal solely for optional presentation.
- No gameplay service capability, timing, request, RPC, or vehicle mutation code is changed by this correction.

PRIMARY RISKS
- The simplified preview loses the decorative workshop scene and graphical damage dots.
- Static and compile checks cannot prove the native heap fault is resolved; the exact damaged-vehicle reproduction still requires a fresh runtime test.

REQUEST INTERPRETATION
- Release stability takes priority over optional preview decoration. Keep the usable preview and service information while removing the two native-risk additions executed immediately before the crash.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Make damaged-vehicle Service entry safe without reducing the actual repair, refuel, rearm, full-service, or cargo functionality.

## 2026-09-03 vehicle-workshop environment correction

REQUIREMENTS
- Vehicle Service must render its dedicated vehicle workshop, not the Arsenal soldier stage.
- The restored workshop must not reintroduce private-world gameplay entities, replicated lamp hierarchies, overlapping global post-processing, or live damaged-vehicle collider sampling.
- Preserve the selected vehicle preview and all Service Bay functionality.

MINIMUM COMPONENTS NEEDED
- One preview-safe vehicle environment containing only the world rig, bounded post-processing, direct lights, the authored vehicle anchor, and render-only decoration markers.
- The existing `SCR_PrefabPreviewEntity` path for the selected vehicle and workshop decorations.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- Reusing `GRSA_StageEnvironment.et` is rejected because it is visibly the soldier/weapon Arsenal scene.
- Restoring the former workshop prefab unchanged is rejected because it recursively spawned gameplay and replication components into the private preview world.
- No service operation, capability, timing, authority, or cargo logic is part of this visual correction.

PRIMARY RISKS
- A decoration whose base prefab has no previewable mesh will be absent rather than spawned as a gameplay entity.
- The exact workshop framing and lighting require a fresh in-game visual test after compilation.

REQUEST INTERPRETATION
- This is a targeted revert of the incorrect environment substitution, combined with a safe reconstruction of the intended vehicle-specific workshop.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Restore the recognizable vehicle Service Bay environment without restoring the native crash surfaces removed in the prior correction.

### Verification

- MCP API and BI Wiki review confirmed the private-world/prefab hierarchy boundary.
- PAC1CLI verified the native `SCR_PrefabPreviewEntity` implementation and the mesh resources used by the safe workshop reconstruction.
- Addon prefab validation passed. Its only two reported warnings are existing API-index limitations in unrelated editor-attribute files.
- Workbench automatically reloaded the current Game scripts at 02:32:00. Script validation completed in 1496 ms with zero errors and the existing 14 base-game obsolete-API warnings.
- Runtime workshop appearance and the damaged-vehicle crash reproduction remain for hands-on verification.
