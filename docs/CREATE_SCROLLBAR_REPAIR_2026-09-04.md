# CREATE scrollbar drag repair

REQUIREMENTS
- Clients can grab and drag the CREATE scrollbar in both directions.
- Preserve wheel scrolling, pooled rows, filtering, and saved browsing position.
- End dragging on release, focus loss, list changes, panel hiding, and shutdown.
- Normalize CREATE catalog labels and status text to 18 reference pixels; never shrink or wrap a row to fit a long name.
- Fit names with an ellipsis at the measured available width and show the complete name on hover, including folders and setup actions.
- Add explicit Black and White accent choices and persist the selected choice across sessions; hue selection restores the normal color mode.

MINIMUM COMPONENTS NEEDED
- Existing CREATE handler and layout: a full-height native track button with a positioned thumb and the existing row-offset calculation.
- Existing hover card and row labels, with measured text fitting after layout.
- Existing theme profile, accent application, and preset swatch row, with one stored monochrome mode.
- Native script validation and structural layout checks; the user performs further in-game checks.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No new scrolling framework, server messages, settings, or dependencies.

PRIMARY RISKS
- Decorative images do not provide native button input handling; consuming mouse-down can prevent native handling.
- Layout scaling, release outside the thumb, and changing the list during a drag.

REQUEST INTERPRETATION
- This is client-local browsing input; asset placement and authoritative gameplay stay on their existing paths.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Make the visible CREATE scrollbar a usable drag control while preserving the existing catalog browser.
- Keep this text normalization focused on CREATE browsing rows and associated descriptions, the area identified in the request.

Evidence references: Enfusion MCP widget APIs; BI Widget/WidgetFlags documentation; PAC1CLI inspection of the native SCR_ScrollBarHandleComponent input lifecycle.

Final implementation adjustment: use a fixed-width, full-height button for the scrollbar track and a positioned, cursor-transparent thumb. This gives input and rendering the same bounds and removes the changing button size from dragging. The user took ownership of further in-game testing; do not launch further playtests or send computer-use input.

IMPLEMENTED
- A 14-pixel track accepts thumb drags and track clicks. Dragging preserves the initial grab offset, clamps at both ends, and clears on release, focus loss, list changes, hiding, or shutdown.
- All 22 pooled CREATE rows use fixed 18-pixel labels and status text. Long labels are measured after layout and shortened with three dots without cutting through a UTF-8 character. Full names remain in search and hover cards; folders and setup actions also have name previews.
- Black and White are labeled presets beside the six existing colors. The theme saves the monochrome mode while retaining the previous hue; choosing a hue returns to colored accents. Older profiles default to hue mode.

VERIFICATION
- Workbench script validation passed with zero errors and 14 warnings.
- Structural checks passed for balanced layout braces, unique object IDs, matching scrollbar/preset widget names, and all 88 CREATE font-size/minimum-size entries set to 18.
- The final track implementation and final ellipsis/hover changes have not been tested in-game. Earlier attempts still had drag/label defects and are not acceptance evidence for this revision. Black/white persistence is implemented but awaits a real reopen/restart check.
- No new playtest or computer input was initiated after the user took ownership of testing. No dedicated-server, remote-client, or JIP result is claimed; these changes are client-local presentation and profile settings.

USER CHECKS
1. Open CREATE and expand a folder with more than 22 entries. Grab the scrollbar thumb, drag down and back up, then release outside the track. Move the mouse again: the list should stay still. Wheel scrolling should still work.
2. Browse long asset names and setup actions. Names should stay at one size on one line and end with dots when needed. Hover to read the full name.
3. Open GM options and select Black, then White beside Accent colour. Close and reopen the options, then restart a session to check persistence. Select a colored preset or move the hue slider to return to color.

API references: [TextWidget sizing](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfaceTextWidget.html), [OverlaySlot padding](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfaceOverlaySlot.html), and [string operations](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfacestring.html).
