# Changelog

## Unreleased

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
