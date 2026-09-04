# CREATE Browser Modernization Plan

REQUIREMENTS
- Replace the stretched collapsed-folder presentation with a denser, modern hierarchy that uses the available CREATE region without distorting individual rows.
- Implement the fixed-row browser geometry now; the requester confirmed that a plan-only response is not sufficient.
- Preserve search, category filters, faction filters, counts, hierarchy, scroll reach, expansion persistence, placement status, and the fixed `Spawn vehicles crewed` / World Capacity footer.
- Show a third-party puzzle control immediately beside the Lightning category control only when non-canonical factions are present.
- Keep canonical APP-6 faction controls in the faction row and use the puzzle control to open the existing third-party faction chooser.
- Repair the Service preview coordinate assertion shown by the requester.

MINIMUM COMPONENTS NEEDED
- Existing CREATE controller, pooled rows, custom scrollbar, expansion state, and third-party faction popup.
- One conditional puzzle button in the category strip; no new catalog or faction subsystem.
- One compact fixed-height row geometry and optional contextual empty-space treatment for the later modernization pass.
- Native preview-local bounds and preview transform APIs for Service placement.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- Rows will not be stretched to fill unused height. Empty height belongs to the browser viewport and future contextual content, not individual folder hit targets.
- No new icon pack, web UI layer, or parallel catalog model is introduced.
- Recent-assets and selected-folder summary content remain optional future work; they are not required to correct the current tree geometry.

PRIMARY RISKS
- Moving the custom-faction entry must not consume or distort an APP-6 faction slot.
- The puzzle control must disappear when no third-party faction exists and must retain selected-custom-faction indication when the chooser closes.
- A preview entity exposes local preview bounds; treating them as live world bounds can generate sentinel coordinates and assert when setting its origin.

REQUEST INTERPRETATION
- "Beside the lightning icon" means a seventh conditional control in the top category strip, not the final slot in the APP-6 faction strip.
- Modernizing the browser means improving information density, hierarchy, and selection feedback while preserving the narrow Bifrost panel and its native visual language.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Fix the Service assertion, relocate third-party faction discovery to a conditional puzzle control beside Lightning, and implement compact stable CREATE rows without moving the footer or replacing the catalog system.

## Recommended layout

```text
┌ CREATE · ALL ─────────────────────────┐
│ [folder][man][groups][objects][gear][⚡][puzzle*] │
│ ALL  [APP-6] [APP-6] [APP-6] [APP-6] │
│ Search assets…                        │
├───────────────────────────────────────┤
│ ▸ Groups                         83   │  24-unit fixed row
│ ▸ Men                           173   │
│ ▾ Objects                     1,169   │
│   ▸ Auto                      1,022   │
│   ▸ Vehicles                    140   │
│   ▸ Weapons                       7   │
│ ▸ Systems                       103   │
│ ▸ Effects                        17   │
│                                       │
│ Context area: recent assets or         │
│ selected-folder summary; never stretch │
│ the folder rows to consume this space. │
├───────────────────────────────────────┤
│ [ ] SPAWN VEHICLES CREWED             │
│ WORLD CAPACITY                        │
│ …                                     │
└───────────────────────────────────────┘

* Puzzle is present only when third-party factions exist.
```

## Proposed interaction model

1. Keep every folder and asset row at a fixed 24-unit height with the complete row as its hit target. This preserves all 22 pooled rows inside the supported compact viewport.
2. Give folder rows a subtle darker band, a fixed disclosure chevron column, a fixed 22 px icon column, a flexible label, and a right-aligned count/status column.
3. Keep hierarchy indentation to one 12 px step per level and draw a low-opacity guide line for expanded descendants.
4. Use accent color for the active row border/chevron and a darker hover fill; do not resize icons or rows during hover or selection.
5. Reserve unused viewport space. In a later approved pass, populate it with either recent assets or a selected-folder summary without changing the tree position.
6. Keep the footer fixed. The tree scrolls independently and never pushes `Spawn vehicles crewed` or World Capacity off-screen.

## Implementation phases

1. Conditional puzzle relocation and Service assertion repair. **Implemented.**
2. Fixed-height row geometry in the existing pooled-row layout. **Implemented.**
3. Clear selected/hover/focus states and long-label truncation testing at supported UI scales.
4. Optional recent-assets or selected-folder context area, only if approved after the fixed-row version is tested.
