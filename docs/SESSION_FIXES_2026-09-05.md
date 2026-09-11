# Multiplayer presentation and property editing review — September 6, 2026

Follow-up: [September 6 bug recap and reopened review](BUG_REVIEW_2026-09-06.md) records the added outside-click candidate, 34-check regression, actual evening server/client findings and GRS/Minnensinger integration gaps. The historical checks below are not user acceptance.

REQUIREMENTS
- Character scale must reach ordinary observers and late joiners, and survive movement, actions and teleporting.
- Toggle Visibility must hide the target for other players and remain in effect until restored.
- Property editing must release input and its old session after apply, cancel, native close or shutdown.
- Keep gameplay changes server-authorized, simulation ownership native, and private GM overlays private.
- Distinguish source, compiler, local runtime, dedicated-server, remote-observer and JIP evidence.

MINIMUM COMPONENTS NEEDED
- Existing GM tool relay and editable-entity replicated properties.
- Shared presentation frame-event ownership for scaling and visibility.
- Local visibility claims for attached parts, including overlapping hidden parents and children.
- Existing property-panel and native-dialog lifecycle callbacks.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No replacement networking service, owner-only visual RPC, per-frame network broadcast, or forced character Update call.
- No publication or edits to the already released 1.0.30 checkout.
- No claim that compilation or a GM success message proves another player rendered the result.

PRIMARY RISKS
- Native animation, prediction, equipment hierarchy and streaming can change presentation after a request.
- Reapplying render state must preserve other features' frame events and each part's original visibility.
- Native attribute end events occur before manager cleanup; handlers must not recursively end that transaction.
- Engine-level character/vehicle scale, collision and third-person rendering still require multiplayer acceptance.

REQUEST INTERPRETATION
- Deeply trace and correct all paths of the reported scale, invisibility and command-editing features, including the teleport and attachment interactions that can undo their result.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
A successful GM request must produce the intended shared world result for relevant players, with retained state when entities stream in later, and must leave the GM editor usable.

## Checkout and evidence

Patch: `C:\Users\Bryce\Documents\Bifrost-Fixes\session-2026-09-05`, branch `bishop/fix-session-replication-input`, based on released 1.0.30 commit `37c79878b7b7d530a147a6a88f2bd512d747a3bd`.

This is the same Bifrost addon identity, GUID `6A0C2D6CE9809C6E`, in an isolated checkout. Workbench's loaded-project query and startup log resolved this checkout's `addon.gproj`. The dirty development checkout and published release remain separate.

The affected session was a remote RPL client. Its scale-3 and scale-1 replies establish server acceptance only. The installed scale script matched 1.0.30; the report was not explained by an older installed scale script.

## Confirmed source defects and corrections

1. **Toggle Visibility was local-only.** `DCO_GMContextMenuBridge.ID_VISIBILITY` called `DCO_GMTools.ToggleVisibility`, which directly cleared local VISIBLE/TRACEABLE flags and stored the target in a local array. There was no server request, replicated hidden property, persistence callback or late-join path. The earlier investigation's statement that Bifrost had no character visibility control was incorrect.
   - The action now uses the existing reliable player-controller server relay, whose server receiver checks GM rights.
   - The server changes an unconditional replicated hidden property on the editable target. The property callback applies local render state on each receiving peer.
   - Frame retention catches flags restored by actions and newly attached equipment. Detached parts release their hidden claim. Shared claims prevent restoring a child while another hidden parent/child still owns it.
   - Restoring visibility restores the original local VISIBLE state. Traceability, collision and damage handling are not changed by invisibility.

2. **Scale retention requested POSTFRAME alone.** BI documents FRAME as the event subscription that keeps entities simulated and their rendering bounds updated. The replicated scale itself has no owner-only or GM-only delivery condition.
   - Active scaling now requests FRAME and POSTFRAME on all supported scaled targets, not just locally controlled characters.
   - Scale and invisibility share event ownership. Resetting scale does not stop active invisibility, and showing an entity does not stop active scaling.
   - Only event bits added by this feature are cleared. No explicit character Update or OnTransformReset was added.
   - This addresses an identified presentation lifecycle gap; it is still a candidate explanation for the remote character-scale report, not a demonstrated remote-client resolution.

3. **Marked-unit teleport bypassed native ownership.** `TeleportEntityTo` directly teleported on the server and called `Update` even for characters. Native editable transform code explicitly sends character teleports to the simulation owner and warns against that Update call.
   - The marked-unit path now validates the destination and uses native `editable.SetTransform`, preserving native vehicle-exit and ownership handling.
   - The separate player teleport and mission teleporter paths were traced independently; they use native player teleport handling and character/boarding checks.

4. **Property sessions did not observe native end events.** The custom panel subscribed to start only, and deferred native-dialog handoff could survive closing its dialog.
   - Confirm/cancel now closes the custom panel and backdrop, releases focus, clears stale attribute/option references and removes deferred refreshes.
   - Shutdown unregisters all three native invokers and cancels the pending conditional refresh.
   - Native-dialog close cancels handoff; a closed dialog cannot reacquire input through a late update or complete an old handoff.
   - Native manager cleanup remains authoritative; the end callback does not recursively confirm/cancel it.

## Path-by-path review

| Vector | Source result | Runtime evidence still needed |
| --- | --- | --- |
| Scale from selected object, world click or player delegate | IDs resolve on server; delegates resolve to their controlled character; duplicate/nested targets are handled there | AI character, player character, vehicle and prop seen by a separate ordinary player |
| Scale authority and rejection | GM rights at request receiver; replicated target and finite range checks before setter | Non-GM rejection and deleted-target request |
| Scale after walking, stance, ragdoll and actions | Persistent field plus both frame phases; native character prediction remains owner-only | Rendered scale and movement observed while active |
| Scale reset combined with invisibility | Shared event ownership keeps whichever feature remains active | Scale 2 + hidden, reset 1 while hidden, then show; reverse the reset order |
| Scale late join / stream out and back | RplProp state and callback have no GM/owner filter; no transient RPC is used as state storage | New joiner and distance-based stream-in render current size |
| Health and movement associated with scale | Max-health change remains server-side; movement factor is replicated and extra movement is simulation-owner-only | Injury/max-health UI, AI movement, player prediction, collision at small/large scales |
| Third-person camera | Local camera adjustment is derived from the local controlled character's scale | First/third-person switch, vehicle entry/exit, clipping |
| Visibility request | Previously local-only; now reliable GM-authorized server request and replicated hidden state | Ordinary observer and a second GM agree on hide/show |
| Visibility after teleport/actions | Hidden state reapplied in frame callbacks | Marked-unit teleport, player teleport, endpoint teleport, stance, animation, firing and respawn boundary |
| Equipment added/removed | Traversal claims new children and releases detached children, preserving prior visibility | Equip, switch weapon, drop, pick up and stream equipment |
| Overlapping hidden hierarchy | Reference-counted local claims preserve the original state until the final claimant releases | Hidden vehicle plus independently hidden attached character; restore in both orders |
| Target deletion / stream-out | OnDelete releases visibility claims; no global ticking service or orphan timer added | Delete hidden targets; stream out/back; disconnect/rejoin |
| Defend group property | Native attributes default to server extraction/write; defender setter checks server and replicates its flag; AI mailbox work is server-only | Change/reopen from a remote GM, observe AI behavior from an ordinary player |
| Defend area and artillery command placement | Orders forward native command prefabs and recipient groups to native placing; no client-spawn replacement | Place/edit/cancel repeatedly, including unavailable recipients and two GMs |
| Mortar emitter settings and firing | Native server attributes; replicated settings; server-only salvo ticks and shell spawn | Remote setting readback and matching shell/impact result |
| Mortar cosmetic impacts | Reliable broadcast on the emitter; emitter prefab is non-streamable; client particles/audio, no replay of completed impacts on JIP | Ordinary player sees/hears impacts; cosmetic rounds cause no damage; LIVE does |
| Native property fallback / close / cancel | Unsupported layouts remain native; handoff is gated by the open dialog; end events clear custom ownership | Escape, Apply, native close, target deletion and rapid reopen without relog |
| GM privacy | World effects are shared; editor overlays, selection cues and panels remain permission-gated | Ordinary player receives no GM-only overlays or controls |

Invisibility is scoped to the selected entity and its attached visuals. It does not promise suppression of separately spawned muzzle effects, sounds, projectiles, or a newly respawned character with a different entity identity.

## Verification

- Production WORKBENCH validation passed: zero errors and 14 existing base-game warnings.
- Layout verification passed: 363 bindings across 26 layouts; both fault-injection checks passed.
- Field resource verification passed: six audio source hashes, graphs/platform metadata, nine settings, teleporter and hint syntax.
- Diff whitespace check passed.
- Native property-session regression passed all 25 checks in a disposable Workbench edit-mode instance: confirm/cancel, repeated reopen before old callbacks are due, and shutdown. This verifies session/queue behavior without visible widgets; it is not an in-game UI or network test.
- The first queue check incorrectly treated GetRemainingTime as immediate proof of cancellation. A separate callback check showed the engine retains cancelled entries until due. The final test advances the isolated queue and checks that stale callbacks cannot modify a reopened session.
- Temporary Game/WorkbenchGame test copies were removed. Production-only validation then passed again with zero errors and the same 14 warnings.
- Both temporary Workbench processes were closed and verified absent; no test handler remains in the shipped script folders.
- Dedicated-server rendering, a separate observer client, JIP, and engine character/vehicle scaling are **not yet verified**. No release-ready claim is made.

Engine evidence: [Entity Activeness](https://community.bistudio.com/wiki/Arma_Reforger:Entity_Activeness), [Game Master entity properties](https://community.bistudio.com/wiki/Arma_Reforger:Game_Master:_Entity_Property_Creation), and installed native sources for SCR_EditableEntityComponent, SCR_EditableCharacterComponent, SCR_AttributesManagerEditorComponent and SCR_BaseEditorAttribute. Native code was read through the Enfusion MCP; replication state and callbacks were checked against its replication overview documentation.

## Multiplayer acceptance run

Use the exact same candidate on the dedicated server and all clients. Use a remote GM, an ordinary observing player, and a third player who joins after settings are applied.

1. Scale an AI and a player character to 2, then 0.5. Walk, crouch, go prone and animate. Both existing clients must see the size; the late joiner must see the current size on first stream-in. Repeat at a distance that forces stream-out/in.
2. Hide a character through Toggle Visibility. The observer must see the body and attached equipment disappear. Teleport via each supported route, change stance/actions, swap and drop equipment. Show it again and verify restoration. Repeat on a vehicle and with overlapping hidden targets.
3. Combine scale and invisibility, restore each in both orders, then delete a hidden target. No lingering hidden equipment or active presentation updates should remain.
4. Repeatedly edit Defend, native artillery commands and mortar-emitter settings. Apply, cancel, Escape and immediately reopen. Test deleting the target from another GM. The editing GM must retain camera, selection, placement and menu input without relog.
5. Observe AI execution and mortar results from the ordinary client. Verify cosmetic versus LIVE behavior, current state for the late joiner, and no GM overlays exposed.

Record server/GM/observer/joiner build identity and evidence separately. A successful request notification or scalar getter on the GM is insufficient to close a rendering failure.
