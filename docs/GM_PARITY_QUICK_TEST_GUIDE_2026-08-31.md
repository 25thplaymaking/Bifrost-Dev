# Bifrost GM changes: quick test guide

Use this list from top to bottom. Each section says where to look, what changed, and the shortest useful test. If a step fails, report the section number, whether the session was local or dedicated, the client role, and the exact action that preceded the failure.

For the current feedback pass, run **section 1 (Compositions)** and then **section 7 (Vehicle Arsenal service bay)**. Sections 2-6 and 8-9 are the broader GM regression pass and can follow afterward.

## 1. Compositions

**Where:** GM world view, select editable objects, then right-click and choose **Save Selection as Composition**. To browse saved entries at any time, use **Open Composition Library** in the action menu. CREATE intentionally has no composition shortcut.

**Changed:** the former bundled action was replaced by a centered Bifrost-styled **Composition Library** with three explicit stages: capture, choose, and place/manage. It includes a saved-count summary, selected-row treatment, pagination, an empty state, status feedback, and labeled fields with persistent examples. Captures are stored in the server profile as the versioned `$profile:BifrostGM_compositions.json` library, so they remain available across sessions on that server. The server validates GM rights, selected replicated entities, prefab availability, hierarchy, and placement bounds. A failed placement rolls back the entire spawn.

**Test:**

1. Select two or more props, right-click the selection, and choose **Save Selection as Composition**. Confirm the panel reports the selected count and puts typing focus in the name field.
2. Enter name/category/author, choose **Save Selected**, confirm the new row shows the name and object count, select it, and choose **Place in World**.
3. Place enough entries to use the library pagination, then close and reopen the panel.
4. Start a later mission or restart the world/server, open the context menu > **Open Composition Library**, and confirm the entry remains.
5. Place it again, then choose **Undo Last Place**.
6. Copy the JSON library to a second server profile with the same addons and confirm the composition is available there.

**Expected:** the workflow is readable without opening another menu, the selected row is unambiguous, and every library control has Bifrost hover feedback. At compact viewport size the panel remains centered and unclipped while omitting only its long help footer. The composition persists after restart, all objects retain their relative arrangement, and undo removes only that GM's latest placement. A server missing any captured prefab refuses the placement without leaving partial objects.

## 2. Multi-selection actions

**Where:** GM world view, select a mixed set of AI, groups, vehicles, and props, then right-click.

**Changed:** supported actions normalize the selection by target category, execute server-authoritatively, skip unsupported targets, and protect player characters.

**Test:** mix one AI, one player, one vehicle, and one prop; run a batch-capable AI action.

**Expected:** the AI changes, the player is protected, unsupported targets are skipped, and the result reports what happened.

## 3. Marker library, areas, and intel

**Where:** GM marker tools.

**Changed:** paginated local/server marker library; point, area, objective, warning, route, and intel types; edit/delete; size and rotation; visibility-awareness controls.

**Test:** create one local marker and one server area marker, edit both, exceed one page, then join with a second client.

**Expected:** the local marker stays local; the server marker, dimensions, rotation, and name replicate and survive a late join without duplicate rows.

## 4. AI and world controls

**Where:** right-click supported selected entities/groups.

**Changed:** door and light state, surrender/release, garrison/ungarrison, fire behavior, speed, stance, and manual hold/release use server-authoritative paths.

**Test:** surrender and release an AI group; garrison and ungarrison it; toggle a nearby supported door and light.

**Expected:** state changes are visible to remote clients, players are not affected, and unsupported entities return a clean result.

## 5. Orders and completion radius

**Where:** group order controls and order attributes.

**Changed:** Scout, Wait, Load, and Unload use native waypoint behavior. Completion radius drives both completion logic and the white order-circle radius.

**Test:** place a movement/wait order, change its completion radius twice, then issue Scout, Load, and Unload orders.

**Expected:** the white circle resizes immediately with the configured radius and AI completes the order inside that same boundary.

## 6. Reinforcements and ambient effects

**Where:** GM placeable entities.

**Changed:** reinforcement task zone plus replicated Campfire, Heavy Smoke, Electric Sparks, and Fireflies presets.

**Test:** place each effect and one reinforcement zone; modify, disable, and delete one effect.

**Expected:** clients render effects, dedicated servers retain authority without trying to render them, and removed/disabled effects disappear for every client.

## 7. Vehicle Arsenal service bay

**Where:** GM placeables, place **Bifrost Vehicle Service Bay**, park inside its 9 m white circle, stop, exit, and use the separate **Vehicle Service Access Point** marker.

**Changed:** the interaction marker can be moved directly or through the explicit GM workflow: right-click the bay or its access marker, choose **Move Vehicle Service Access**, then click the desired point. The server clamps the result inside that bay's service circle and replicates it for remote/JIP clients. Vehicle Service reuses the GRS shell and vehicle stage with exactly two modes: **Repair** and **Cargo**. Repair reads native replicated hit zones, projects repair-point markers, and offers Repair, Refuel, Rearm, and Full Service from the footer. Rearm only refills the already-mounted authored magazines and rocket barrels on the server; it never replaces a mounted weapon, feed, ammunition type, or prefab. Cargo retains the searchable GRS contents list. The server revalidates the user, marker, vehicle, range, motion, capability, and storage capacity before applying a completed operation.

**Test:**

1. Place two bays. Right-click each bay, choose **Move Vehicle Service Access**, and click a different position near the edge of its own circle. Attempt one placement beyond the circle and confirm it clamps to the nearest valid point. Press Escape during another move and confirm it cancels.
2. Move one access marker onto a separate static prop inside its bay. Drive into the bay. Confirm the action is unavailable while seated, then stop, exit, walk to the prop-mounted marker, and open it. Confirm opening the service screen does not crash or report layout-parser errors. The header must show only Exit and **Service Bay**, and the mode strip must contain exactly **Repair** and **Cargo**. Confirm there is no Armaments tab or ammunition quantity/type control.
3. Confirm the default camera faces the vehicle's front. Press and hold the primary mouse button on unobstructed stage pixels, drag left and right, and verify the camera orbits while the vehicle remains fixed on its authored anchor. Repeat the drag after starting over a transparent repair-marker area. Use the mouse wheel or controller zoom, then press Reset View; Reset View must restore the front-facing camera and authored vehicle orientation. Confirm the vehicle remains clear of the floor, walls, roof, and props from the reachable views. Confirm repair equipment, side/rear crate stacks, tool boxes, a floodlight, and two in-place working mechanics are present, while the camera-to-vehicle corridor remains clear and no Arsenal gun table, gun wall, or shelf set appears. Check the front, both sides, roof, wheels, and rear for readable balanced lighting rather than a black silhouette or blown-out material response.
4. Damage several vehicle systems. In **Repair**, confirm raw `UNKNOWN`, `UBX`, `UCX`, and `FG` collider prefixes never appear. Confirm every visible repair point has a white marker; select several rows and verify the highlighted marker and white leader line follow the correct part while the detail text reports its current health, damage percentage, state, and fire condition before repair.
5. Drain fuel and expend mounted ammunition, then run Repair, Refuel, Rearm, and Full Service. Confirm state changes only after the authoritative operation completes. After each operation, close Service, leave the circle, and confirm the same vehicle can still be opened, entered, driven, aimed, and fired. Rearm must retain the original mounted weapon prefabs and restore only their authored ammunition capacities.
6. During progress, move the vehicle, re-enter it, walk away, and leave the circle in separate attempts.
7. In **Cargo**, confirm **Gear & Ammo** opens by default as a searchable list. Exercise All, Packed, Magazines, Attachments, Throwables, Explosives, Medical, and Equipment filters; add and remove entries with +/- near capacity and verify packed counts only change after the server response. Select **Weapons**, verify that only the weapon carousel appears, add one weapon, then return to **Gear & Ammo** without closing Vehicle Service.
8. Put a partially empty handheld weapon in vehicle cargo, run Rearm, and confirm that carried weapon remains unchanged.
9. Close Vehicle Service with Exit and reopen it three times, including one immediate interaction after close; repeat once with Escape. Confirm every visit creates one preview only, the camera still responds, and no crash or frozen cursor occurs. Then repeatedly open and close native properties for an item, player, and system. Include Escape, Cancel, Confirm, and one rapid close/reopen cycle; verify GM input always returns without leaving Game Master.
10. Repeat from a remote client on a dedicated server, then join late and inspect the unchanged mounted weapons and moved access marker.

**Expected:** the preview uses the established GRS visual language and controls; repair points remain attached while the camera orbits or zooms; only the selected stopped vehicle changes after the timer; cancel conditions cause no mutation; current mounted magazines and rocket barrels refill without replacing their authored weapon objects; cargo weapons are never treated as mounts; the vehicle remains interactable and operable after service and after leaving the area; and damage, fuel, ammunition, cargo, and access-marker state agree for server, remote clients, and JIP.

**Authoring the preview placement:** open `Prefabs/UI/GRSA_VehicleStageEnvironment.et` in Prefab Edit, select the nested `GRSA_VehiclePreviewAnchor` entity in the hierarchy, move it to the desired floor point, rotate its yaw to define the vehicle's saved forward direction, and save the environment prefab. Vehicle Service centers every preview vehicle over that anchor, floor-aligns it, and derives Reset View from the saved yaw; no script constants need editing.

## 8. APP-6 and GM visual overlays

**Where:** Create panel, placed squads, selected group paths/orders, and rotated vehicle bounds.

**Changed:** APP-6 create icons preserve aspect ratio; base-game squad symbols update from current composition and echelon; modded faction art is retained; order/action cues and debug-style visuals use the replicated render path.

**Test:** place a base squad, add/remove members or role types, assign an order, select it, and rotate a vehicle.

**Expected:** the squad icon changes with its actual size/composition, the path ends at a readable action glyph, and the vehicle bounds follow rotation.

## 9. Arsenal and Gunsmith regression pass

**Where:** Arsenal Access Soldier and Gunsmith tabs.

**Changed:** carousel selection, clothing/container contents with fit feedback, compatible-magazine loading, editable item attachments, Soldier-tab save confirmation, improved navigation, and weapon/table clearance.

**Test:** switch weapons through the carousel; load/unload a vest and pants; edit an attachment-capable non-weapon item; switch from an unsaved gun edit to Soldier; inspect an AK with a long magazine.

**Expected:** selections never strand the UI, capacity updates after each confirmed add/remove, attachment controls remain usable, the save confirmation is visible, and magazines do not clip through the table.

## Defect report template

Guide step:

Local or dedicated:

Host, remote GM, or ordinary client:

Exact clicks/actions:

Expected:

Observed:

Repeatable after reopening the menu:

Screenshot/log timestamp:

The long-form evidence and edge-case matrix remains in docs/GM_NATIVE_ZEUS_PARITY_AUDIT_2026-08-31.md.
