# Changelog

## Unreleased

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
