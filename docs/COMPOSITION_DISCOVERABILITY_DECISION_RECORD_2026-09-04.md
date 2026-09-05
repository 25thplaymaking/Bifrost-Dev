REQUIREMENTS
- Make reusable compositions straightforward to discover and use from the normal Game Master workflow.
- Match the practical Arma 3 flow: select the objects that form a base, save that selection with reusable metadata, then open a composition library and place it in a later mission.
- Keep the existing persistent server library, authoritative capture/placement validation, atomic placement rollback, and remote/JIP snapshot path.
- Make both saving a selected set and browsing saved compositions available without requiring an undocumented empty-ground right-click.
- Preserve the current Bifrost visual language and existing CREATE panel behavior.
- Bring the Composition Library itself to Bifrost's full and compact viewport standards, including responsive sizing and consistent hover feedback for every control.

MINIMUM COMPONENTS NEEDED
- The existing composition panel, service, RPC, persistence, and placement target flow.
- One always-visible COMPOSITIONS control in the CREATE header for opening the library.
- Two selection-aware entries in the existing Bifrost action menu: Save Selection as Composition and Open Composition Library.
- A capture-focused open state that reports the selected-object count and focuses the composition name field.
- The existing Bifrost viewport polling and hover systems applied to the composition panel; no parallel UI framework is needed.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No requested behavior requires a second composition format, a parallel persistence service, or authoring fixed prefab compositions in Workbench.
- A live placement ghost/rotation system is not added in this correction because the existing terrain-click placement already satisfies reuse; it can be evaluated separately if hands-on testing shows placement control is insufficient.

PRIMARY RISKS
- The action menu receives both hovered and selected state; the save entry must be based on the complete selected set rather than only the hovered object.
- Opening a modal panel from CREATE or a context action must not strand search/menu focus or leave an active placement mode behind.
- Static and Workbench validation cannot prove dedicated-server persistence, remote-client capture/placement, or JIP library delivery.
- The library is a dense vertical workflow; compact presentation must preserve capture, library selection, placement, status, and pagination while treating the long help footer as optional guidance.

REQUEST INTERPRETATION
- A Game Master selects every editable object making up the hill base, chooses Save Selection as Composition, supplies name/category/author, and captures it.
- In a later mission, the Game Master opens COMPOSITIONS directly from CREATE, selects the saved entry, chooses PLACE IN WORLD, and clicks the intended landmark.
- The saved library belongs to the server profile and remains available across missions hosted with that profile and the same addon content.
- Bifrost-standard layout means the same full/compact breakpoint as the main GM shell, theme-aware controls, readable hierarchy, and visible hover feedback rather than only parseable layout syntax.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Repair the missing composition workflow by exposing the already-authoritative system where Game Masters naturally look for it, while retaining the current persistence and multiplayer architecture.
