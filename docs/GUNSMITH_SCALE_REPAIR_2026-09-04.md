# Gunsmith input and GM Scale Object repair

REQUIREMENTS
- Weapon tiles select a new draft weapon and refresh its preview.
- Attachment slots open compatible choices; selecting a choice updates the draft.
- GM Scale Object applies supported changes on the server and gives an actionable rejection for unsupported selections.
- Preserve server authority, remote replication and join-in-progress state. Leave hands-on testing and Workshop publication to the user.

MINIMUM COMPONENTS NEEDED
- Existing preview drag boundary, item-row activation event and carousel focus handling.
- Existing mission scale validation, replicated state and result message.
- Existing Workbench compiler and release manifest workflow.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No new input system, preview architecture or network channel.
- The user confirmed a barricade and a character. Enable static assemblies; retain the character exclusion with an explicit explanation because correct movement and collision scaling are not established.

PRIMARY RISKS
- Workspace mouse events can obscure the original widget. Preview drag must check the actual hit widget.
- Mouse and keyboard activation must not deliver duplicate row actions or turn quantity edits into selections.
- Mouse focus must not slide a tile away between press and release; gamepad focus still reveals tiles.
- Broadening scale support without accounting for child entities or physics can produce inconsistent geometry.

REQUEST INTERPRETATION
- Scale refers to GM Scale Object. The Arsenal failures concern selection and attachment-slot activation.
- The user cleared the Workbench crash as a false positive and retained game testing and BI upload.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Correct the three reported failures within the existing systems, validate scripts and source integrity, and include the corrections in the authorized 1.0.29 main release.

## Source evidence and changes

- Native `SCR_ButtonBaseComponent` emits `m_OnClicked` from both mouse `OnClick` and menu selection; the custom row previously forwarded only `OnClick`. The row now bridges the native event once and removes its subscription on detach. Hidden quantity controls no longer mask row clicks.
- Preview drag eligibility now checks both the event source and `WidgetManager.GetWidgetUnderCursor()`. A workspace-level callback cannot bypass the button/slider/list exclusion.
- Mouse focus no longer starts a carousel glide before release; gamepad focus still reveals the focused tile.
- Native `SCR_EditorLinkComponent` spawns barricade parts with the composition root as their physical parent. Scale eligibility now validates that hierarchy instead of rejecting all children and model-less roots. The server sets the root scale, retains the existing replicated property, and refreshes transforms on each machine. Characters, vehicles and moving physics remain unsupported and receive explicit reasons.
- Consulted Enfusion MCP, the BI Modular Button tutorial, and native scripts/prefabs through PAC1CLI. Native source was used as API evidence, not copied into this implementation.

## Validation

Final Workbench `WORKBENCH` script validation passed on September 4, 2026 at approximately 22:45 EDT: zero errors and 14 existing base-game deprecation warnings. The whitespace check passed. No live reload, playtest or computer-use operation was performed. Weapon selection, attachment interaction, barricade collision, remote-client and JIP checks remain operator acceptance items in TESTING.md and the Mission Tools guide.
