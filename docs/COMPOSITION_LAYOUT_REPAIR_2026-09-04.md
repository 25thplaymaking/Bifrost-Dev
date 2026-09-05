# Composition Library layout repair

REQUIREMENTS
- Correct the cramped controls, unreadable capture button, touching labels and excess blank panel space shown in the user's in-game screenshot.
- Make saving selected objects and choosing/placing saved compositions clear, retaining category, author, paging, undo, delete, theme and Back behavior.
- Preserve the existing server-authoritative composition implementation and unrelated working changes.
- Validate actual engine widget geometry as well as compilation; report visual/runtime evidence separately.
- Follow-up: remove the Composition Library shortcut from CREATE and expose the already implemented mission actions in Lightning > Bifrost, with explicit object/ground targeting.
- Follow-up: clearly mark every text/numeric input in both panels, provide persistent reference descriptions/examples and make the instructions easy to follow.

MINIMUM COMPONENTS NEEDED
- The existing composition layout and panel controller; existing context bridge only for modal input isolation.
- Existing CREATE catalog entries, click routing and placement-confirm listener for mission actions; no new placeable backend.
- Labeled fields, correctly expanding horizontal slots, contrasting button plates, separate row metadata and a bounded library scroll area.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No new library backend, persistence format, dependencies or additional screens.
- Server-file portability instructions belong in documentation, not the save/place interface.

PRIMARY RISKS
- FillWeight does not request Fill sizing by itself; controls currently measure to text width.
- The capture button has an opaque accent plate behind an accent label, making its text unreadable.
- Existing fixed sizes grow on compact displays and leave an empty library unnecessarily tall.
- Compile/widget-existence checks alone missed the visible sizing faults; actual widths and separation must be inspected.

REQUEST INTERPRETATION
- Repair the Composition Library in the supplied screenshot, move mission actions into the requested Bifrost folder, and apply the subsequent text-field clarity request to both affected editors.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
Make composition saving and mission setup legible and usable through existing panels and catalog routing, preserving server authority.

## Changes
- Rebuilt the existing resource with explicit Fill sizing, persistent Name/Category/Author labels, a contrasting Save Selected button and separate composition name/metadata lines.
- Empty library uses a 620-unit panel height; populated library uses up to 760. Width is capped at 840 and both dimensions are bounded by available viewport space. The library scrolls while the footer remains accessible.
- Place/Delete remain visibly disabled until an entry is selected. Save requires selected objects and a nonblank name. Page navigation is disabled at its limits and hidden for one-page libraries.
- Replaced obsolete cursor coordinates with the selected composition name; placement still chooses its destination after Place In World. Removed file-transfer implementation details from the interface.
- Removed the Composition Library button and binding from CREATE. Context-menu access and Save Selection as Composition remain available.
- Registered all 13 mission operations in Lightning > Bifrost with setup routing, preserving native prefab placement for the original entries. Ground tools request a world position; object tools consume an existing selection or request an editable target. Back/Escape cancels targeting.
- Added composition backdrop and placement/context-menu guards so modal clicks cannot start another world action.
- Added persistent field descriptions/examples and limits to composition Name/Category/Author; optional fields are marked. Mission fields now have contrasting bordered boxes, tool-specific captions/examples and numeric limits, short numbered steps, and action-specific button names. Teleporters explicitly distinguish endpoint name from matching link name. Chatter no longer shows a title field that its speaker attribution would ignore. Empty required text and invalid lengths/numeric ranges receive field guidance before submission; server validation remains authoritative.

## Evidence
- Enfusion MCP LayoutSlot API, BI Layout Editor documentation, and PAC1CLI inspection of the installed native WidgetLibrary button resource informed the sizing and hierarchy changes.
- Native WORKBENCH validation/reload passes with zero errors and the same 14 base-game warnings.
- A temporary hidden engine layout probe measured empty/1/8/9-entry states at 840- and 720-unit panel widths. All required measured widgets were present. Name field widths are 601.599 and 481.599; category/author widths are 355 and 295; Save Selected fits its 104.4-unit text width. Headings and counters are separated; footer controls remain within the panel at all eight tested configurations.
- Fresh local Workbench gameplay shows Bifrost with 21 entries (8 existing plus 13 mission actions), FX with its original 8, and no Composition Library shortcut in CREATE.
- This is local UI evidence. Dedicated-server, remote-client and JIP gameplay acceptance for the mission features remains pending.
- The measurement pass and fresh-game folder check occurred before the final field-description follow-up. That follow-up passes source validation but its final visual/interaction pass remains pending: Computer Use received a physical Escape stop. No further Computer Use input was issued after that stop.
- Final WORKBENCH script validation passed with zero errors and 14 warnings after the field-validation changes. All three affected layouts pass delimiter and unique widget/object identifier checks; Git whitespace checks pass. Temporary Workbench handlers are absent from the addon.

## Remaining direct checks
- At normal and compact UI scales, open both panels and confirm captions, examples, bordered inputs, scrolling and footer actions remain readable without overlap.
- Save a composition with a name, optional category and author; select it and place it. Empty names and overlong fields must explain how to correct them.
- Open each Bifrost setup action. Follow its numbered steps using only visible instructions. Verify required text, radius and scale validation; pair two teleporters using the same link name.
- Repeat authoritative gameplay operations on a dedicated server with a remote GM and join-in-progress client; local UI checks do not establish those outcomes.
