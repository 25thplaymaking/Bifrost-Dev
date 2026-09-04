# Changelog

## Unreleased

- Removed the legacy autonomous DCO AI suite from Bifrost, including morale and suppression, tactical movement and flanking, procedural and bounding movement, contact reactions, autonomous formations and stance control, specialist weapon behavior, idle/merge/standoff logic, and their configuration surfaces. The released pre-extraction source is preserved on `bishop/dco-ai-compat-source-v1.0.28` for a separate compatibility addon.
- Kept GM-issued stance and formation actions on native server-authoritative paths and retained explicitly GM-directed scenario tools.
- Reviewed retained formations, stance, ambush, defence, QRF, directed CQB clearing, and waypoint completion-radius controls. Formation now reaches every active native movement handler, stance and standing formation affect direct AI members only, and each authoritative state transition emits a focused `[DCO-GM]` diagnostic.
- Fixed the QRF waypoint's serialized intent, preserved its rally waypoint while it responds, added support retargeting and stand-down reporting, and allowed defence groups to reorient when the threat direction materially changes.
- Repaired ambush rearming and manual hold-fire coexistence, restored role-specific task-zone colors, and made active CQB building bounds visible to remote Game Masters through the Bifrost overlay.
- Removed the targetless no-op CQB group/context order; directed CQB remains under `COMMAND > TACTICS > Clear Building`. Renamed the targetless QRF toggle to `Enable QRF Response` so it is distinct from placing a QRF rally area.
- Audited layout, prefab, and editor registrations after the extraction and removed the obsolete `Auto (release)` stance action that depended on the extracted DCO stance lock.
- Changed the CREATE asset tree to start fully collapsed for each new match while remembering expanded folders when the GM interface is reopened in that match.
- Moved `Spawn vehicles crewed` directly above `WORLD CAPACITY` and kept every Bifrost-authored utility in the Lightning tab under either `FX` or `Bifrost`.
- Removed the experimental Service Bay Armaments tab and every RPC that changed mounted weapons, ammunition feeds, ammunition types, or authored capacities. Repair-tab rearm now only refills the vehicle's already-mounted authored magazines and rocket barrels on the server, leaving the weapon prefab and mount hierarchy untouched. The nearby Service interaction no longer replaces native vehicle door and seat actions, so closing the menu does not leave a usable vehicle behaving like a prop.
- Made Service Bay operations capability-aware: unarmed vehicles omit Rearm but retain supported Repair and Refuel actions, and Full Service includes only supported stages and their durations. The server now acknowledges the accepted capability mask and duration over a reliable owner RPC before client progress begins, then applies only that authorized capability set. Replaced the private world's full gameplay-vehicle clone with the base game's render-only prefab preview, and corrected its placement to use guarded preview-local bounds instead of invalid generic world-bound sentinels.
- Fixed Arsenal Contents teardown so Back or Escape immediately reopens the originating gear category instead of leaving the left selection panel blank. A newly focused or clicked gear card now cancels any stale restore and replaces an already-open category immediately, preventing Vest results from remaining after Footwear is selected. Assembled ALICE and Soviet harness inventory remains routed through its authored owned pouch storages for matching validation, capacity display, and server-side deposits.
- Restored Loiter as an armed gun-run placeable by default and added focused pass, projectile-load, and spawn diagnostics.
- Added a persistent server composition library with Arma 3-style name, category, and author metadata; world-click placement; hierarchy-preserving terrain placement; atomic rollback; delete; and per-GM placement undo.
- Added editable Situation, Mission, Execution, Signal, and Intelligence entries to a dedicated Mission Info category in Scenario Settings, backed by the native replicated map-journal briefing component and hidden from ordinary entity Properties.
- Restored full AI navigation route rendering, kept the concealed/visible cursor marker on the render frame, removed the redundant visibility-checking message, and suppressed the vanilla perceived-faction hint while Bifrost is open.
- Repaired the malformed Gunsmith receiver-card hierarchy so `WEAPON / SWITCH` opens its carousel again, and repaired the CREATE catalog's per-match expansion-state lifecycle.
- Kept the Arsenal Access `TARGET` status readable without assigning a global fill share that compressed every CREATE row, corrected the crewed-vehicle checkbox border so the GM layout loads without the unsupported `style` diagnostic, and increased the checked `X` contrast against the accent border.
- Removed CREATE's redundant native scroll wrapper and replaced proportional pooled rows with fixed 24-unit rows, so collapsed folders remain compact at the top of the browser while the viewport owns unused space above `Spawn vehicles crewed`. Bifrost's existing paging and scrollbar continue to handle expanded trees. Once the complete Bifrost catalog is built, transient native Objects/search filter notifications no longer rebuild it and remove Vehicles from the All tree.
- Gave all APP-6 faction tabs equal fill slots, reduced excessive icon padding, and retained faction symbols at their authored 3:2 presentation. Third-party factions now expose a conditional puzzle control directly beside Lightning, leaving the faction strip dedicated to canonical APP-6 choices.
- Renamed the user-facing `FX: Ambient` module and property labels to `FX: Emitter` while preserving its prefab GUID and compatibility with existing layouts.
- Suppressed repeated Bifrost catalog translation requests for the three supplied missing base-game explosive-charge and 5.56 mm string IDs by using prefab-derived fallback names.

## 1.0.28 - 2026-08-30

- Added the GRS Arsenal studio with Soldier, Gunsmith, Kits, and Settings screens; permissive all-faction catalogs; configurable scenario restrictions; themed accents; camera controls; studio lighting; and text-safe layouts.
- Added server-authoritative draft, kit, attachment, and loadout application with target, policy, proximity, stream, and payload validation for remote clients.
- Added a paged weapon and attachment carousel, readable attachment labels, conditional callout lines, rail-position controls, Gunsmith leave confirmation, and safer stage placement for weapons and the character.
- Added replicated Arsenal Access placement on multiple world objects, including a movable interaction marker and join-in-progress state.
- Added Player-tab `Mark for Teleport`, reusing the existing Game Master teleport destination flow and server-authoritative transform route.
- Fixed waypoint `Completion Radius` so the server arrival check, replicated value, join-in-progress state, and white order circle all use the same distance.
- Fixed the lower Game Master edit-panel controls clipping into the bottom rail by making the entity list consume only the remaining vertical space.
- Expanded trigger setup, unit linking, synchronization, and finalization while preserving authority-owned state for dedicated servers and join-in-progress clients.
- Removed obsolete Arsenal posing remnants, reduced routine diagnostics, simplified comments and UI language, and verified lazy initialization for cached static state.
- Kept Workbench MCP handlers in the external Enfusion MCP source tree so they remain easy to restore while release packages exclude them.

## 1.0.26 - 2026-08-27

- Added a rightmost custom-faction folder with authored faction artwork, stable faction-key ordering, and pagination for larger mod sets.
- Reworked CREATE catalog refresh and search around the merged native browser and placing registries while preserving the native browser's active filters, search, page, and tab during placement.
- Added immediate multi-token CREATE and EDIT search with prefab, faction, category, type, and subcategory matching.
- Added stable faction-key selection and catalog-driven group prefab selection to triggers, including legacy migration and explicit missing-mod entries.
- Added custom-faction pagination to the EDIT force overview and keyed its ammunition cache by faction instead of visible row.
- Added catalog-driven third-party helicopter selection to Air Support and Loiter FX attributes, with stable prefab persistence and runtime vehicle validation.
- Added stable custom-faction targeting to Loiter FX and retained missing faction selections without silently substituting vanilla factions.
- Removed broad overrides of the native command list, rocket-ammunition base, and attribute layout to reduce conflicts with other loaded mods.
- Retained launcher discipline in scoped runtime logic and removed obsolete serialized AI-world settings that are no longer declared by the shipped component.
- Preserved Bifrost's GM notification suppression, content-browser ownership, and inline attribute-dialog suppression.
- Simplified release validation to compile and static-integrity gates, with practical mod-stack compatibility testing after Workshop publication.

## 1.0.25 - 2026-08-26

- Added a GM-placeable Arsenal Access system that attaches a replicated F interaction to placed items, vehicles, and other editable entities.
- Restored mouse-wheel scrolling and correctly sized content in both Edit Loadout arsenal columns.
- Repaired Escape, close-button, and focus release handling across Bifrost menus so leaving one menu does not block the others.
- Restored dedicated-server AI resume by retaining and reapplying each paused entity's authoritative pre-pause state.
- Routed precise transform previews and commits through the replicated editable-entity path.
- Shortened the CREATE deployment prompt so it remains readable at the bottom of the panel.
- Added per-order completion radius and approach controls, including Tactical, Rush, Charge, DCO Flank, and DCO Covered behaviours.

## 1.0.23 - 2026-08-26

- Added visible draggable scrollbars and reliable wheel capture to both Edit Loadout columns.
- Added the replicated Arsenal Access system for turning any prop location into an interactable arsenal.
- Removed redundant CREATE category asset totals and centered all six category icons.
- Increased tactical overlays and role markers to a 30 Hz client update while caching expensive discovery and trace work.
- Increased dedicated-server AI overlay snapshots to 4 Hz and kept visible targets attached to their local replicated entities.
- Grounded movement routes into an overhead GPS-style path and clipped lines cleanly across the camera plane.

## 1.0.1 - 2026-08-26

- Fixed dedicated-server simulation pause authority, replication, reconnect, and join-in-progress state.
- Restored client-visible precision controls, tactical overlays, AI vision, and navigation paths without diagnostic shapes.
- Fixed right-click and Escape editor lifecycle handling.
- Added paging and authored symbols for modded factions.
- Reduced role marker updates and prevented opacity settings from obscuring text and icons.

- Consolidated the current Bifrost Game Master interface and scenario-editing feature set.
- Added tactical AI, effects, trigger, arsenal, overlay, and precise-transform systems.
- Added complete attribution for bundled third-party icon assets.
- Removed internal research, handoff, cache, and editor-only material from the release tree.
- Removed two unused legacy component scripts and four unreferenced icon variants.
- Replaced legacy product labels and corrected editor registry ownership for Bifrost.
- Licensed Bifrost's original code and content under APL-SA.
- Reworked gunrun projectiles so cosmetic passes create visual tracers without spawning ammunition.
- Reduced verbose and provenance-oriented source comments.
- Split instruction-dense symbol, configuration, and CQB methods and added a release script-budget gate.
- Corrected remote pause, budget presentation, and FX initialization so they follow authority-owned replicated state.
