# Properties text, ownership and setting scope

REQUIREMENTS
- Render localized RGB/RGBA markup correctly, or remove formatting from plain labels; search authored strings and dynamic text consumers across the addon.
- Keep entity Properties, including the logged E_Barricade_S_US_01.et, inside Bifrost; prevent empty sessions from opening native Scenario Settings.
- Show Bifrost settings only for compatible selected targets, preserve dedicated-server authority, and verify session transitions and filtering.
- Preserve all prior workspace changes and use the existing Bifrost-Dev project, GUID 6A0C2D6CE9809C6E.

MINIMUM COMPONENTS NEEDED
- Native rich-text descriptions, one plain-label text helper, the existing Properties panel/dialog handoff, and a native attribute-manager filter.
- Existing native widget regression plus temporary local prefab fixtures using the existing preview-world owner; no scenario save or alternate addon.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No global removal of useful descriptions, replacement game strings, new dependency, background service, or native-source copies.
- Do not reread server-owned attribute values on a client to decide applicability; filter in each attribute's existing server/local collection phase.

PRIMARY RISKS
- Localization expands markup after a plain widget receives its text.
- Native collection uses the union of applicable settings across selected objects; Bifrost-specific controls must use the intersection.
- Empty or data-only sessions still have a native transaction and must close without losing input ownership.

REQUEST INTERPRETATION
- Fix the reported display and scope defects throughout the relevant string and Properties paths, with native evidence and regression coverage.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Properties should show readable text and only the settings appropriate to the current selection, without leaking native dialogs or previous-session controls.

## Initial evidence

- Workspace path and addon.gproj identity verified. MCP live lookup resolved this exact workspace; Workbench reports edit mode in the existing GM_Arland world.
- The active script log records E_Barricade_S_US_01.et placed at 12:58:43.826, then gm.properties.native-fallback and supported=0 at 12:58:44.458-459. This identifies the user's roadblock and the native-dialog fallback.
- CleanDescription stripped markup before translation, so localized Blood descriptions still exposed raw color tags in a TextWidget.
- PAC1CLI's installed SCR_AttributesManagerEditorComponent source includes an attribute when at least one selected item is compatible, and opens its native dialog before broadcasting the completed attribute list.
- The Bifrost list registers 110 unique attributes across 15 files; an additional FPS attribute file is present but is not registered in that list. Component and target checks are being checked against real prefab fixtures as well as the selection filter.

## Implemented fixes

- Properties descriptions and all six notification rows use native RichTextWidget rendering. Blood retains the game's RGBA colour markup. Plain labels, picker options, category names, display names and context-menu text expand localization before removing recognized formatting. Comparison signs and line breaks survive cleaning.
- The string sweep covered authored Scripts, UI and Configs text, including ignored source files, and traced dynamic localized consumers. The only authored colour markup outside the cleaner is the already-rich-text arsenal statistics block. No native string table was copied or modified. This audit checks rendering and routing; it does not claim every possible translation or third-party string was visually reviewed.
- Every received Properties session replaces the previous local snapshot and clears pending category, scroll and dropdown work. The current edited targets determine whether Scenario Settings controls are available. Empty or data-only object sessions show a Bifrost empty state and retain input ownership.
- The native attribute dialog starts hidden while Bifrost takes ownership, preventing its initial visible frame. It remains available when Bifrost is inactive. Standalone teleporter routing remains in place.
- Bifrost attributes must apply to every selected target. The native manager's existing server/local collection phases filter aligned attribute IDs, values and multi-selection states; native settings retain their existing selection behavior. Native server confirmation still validates each target before writing. No gameplay authority was moved into client UI.
- Audio emitters are excluded from the shared live explosion emitter lookup, which previously exposed the Firing and Track Players controls. Both reading and writing use this corrected lookup.
- Removed the unused session-renderability helper and kept new comments local to ownership, localization and server phase decisions. Existing dirty workspace changes were preserved.

## Native source evidence

PAC1CLI inspected installed data007.pak, including SCR_AttributesManagerEditorComponent, SCR_BaseEditorAttribute, EditorAttributesDialogUI, the native attribute layouts and the exact barricade prefab. Compositions.conf identifies the barricade as `{D7B8408B96F4DF79}PrefabsEditable/Auto/Compositions/Slotted/SlotRoadSmall/E_Barricade_S_US_01.et`. The local trigger metadata supplies `{EEFC7B09110761DE}Prefabs/E_DCO_Trigger.et`. MCP API inspection confirms RichTextWidget inherits the plain text widget API used by the notification rows.

## Verification on 2026-09-12

All validation used the authoritative project:
`C:\Users\Bryce\Documents\My Games\ArmaReforgerWorkbench\addons\Bifrost-Dev`
GUID `6A0C2D6CE9809C6E`, internal ID `BifrostDev`. MCP live project lookup resolved this exact directory before authoring/validation. The initial World Editor state was edit mode in GM_Arland; no acceptance claim is based on the later bridge's generic play-mode heartbeat label.

- Native regression: **840 checks passed, no failures**, at 13:16:20 local time. This includes the earlier input-lockup regression, all 110 attributes against four real prefabs, all 110 against mixed roadblock/audio selection, all 110 against audio-only selection, actual Blood localization, plain-text conversion, native rich-text widget types, and repeated empty Properties cleanup. The previous run exposed and confirmed the two audio eligibility defects.
- Native fixtures were local to a private preview world; spawned entities were deleted and the world owner released. The successful run logged path-only GUID warnings for the barricade and trigger. Their test literals were subsequently corrected from the authoritative native catalogue/local metadata; that metadata-only correction was not rerun. It does not change production code or the tested eligibility logic.
- A proposed rich-text glyph-width check returned zero for both coloured and plain text before a rendered frame. It was removed as an invalid headless measurement. Native widget type and localization are tested; actual RGB colour and wrapping remain visual acceptance checks.
- Feature layouts: **364 bindings across 26 layouts**, zero failures, two fault-injection checks passed.
- Attribute layout checks: all three groups passed, including the 15 supported layout families, native-row fallback exclusion and rich-text/label bindings. These are source contract checks; the native probe supplies behavioral evidence.
- All three temporary regression script copies were removed. Native production reload completed. Final MCP ValidateScripts returned **Success=true, Errors=[], 14 base-game obsolete-API warnings**. No Bifrost compiler warnings were reported. Whitespace validation passed.
- One Workbench process remained, with no Python or PAC1CLI validation process left behind. No console window was launched and no MCP configuration was changed.

### Unresolved Workbench reload condition

The existing Workbench process logged `Resources are leaking! Check log!` at GameApp.cpp:1287 during native reload at 13:06:25, 13:08:47, 13:12:15, 13:15:47 and 13:19:33. The first entry precedes the first new prefab-fixture run; the final entry also occurred during production reload after temporary hooks were removed. Logs identify a thermal-profile resource but do not establish the root cause. Compilation subsequently completed and native validation passed; these do not establish that the reload assertion is fixed. Computer-use observation failed with `window crop is outside captured monitor` and then `no screenshot targets found`, so no dialog state or dismissal is claimed.

Save open Workbench work and restart the same authoritative project before acceptance testing. Do not treat this warmed-up process as a clean runtime baseline. Physical input, rendered colour, listen-server behavior, dedicated-server authority, remote clients and join-in-progress remain distinct unverified acceptance levels.

## Reproduction and acceptance actions

1. Save open work, restart Workbench and load this exact Bifrost-Dev addon. Start a fresh disposable GM scenario. Confirm no new reload/resource assertion before testing.
2. Place a character, open Properties and find Blood. Confirm the highlighted `0` and `kill` text renders in colour without literal `<color>`, `rgba=` or closing tags. Check long descriptions at narrow and wide panel sizes and at the normal UI scale. Browse categories, picker options, context labels and notifications for raw markup or unresolved localization keys.
3. Open Scenario Settings with the cog, browse time/date and presets, then close it. From All Objects place the US small road barricade `E_Barricade_S_US_01.et`, open Edit Properties and repeat after another scenario-settings visit. Expect one Bifrost Properties surface, no native Scenario Settings flash, and no mission/time/preset controls. If it has no editable values, expect the empty-state message.
4. Alternate Properties between that barricade, a character, a vehicle, an audio emitter, an explosion emitter, a trigger and a teleporter. Each target must get only compatible settings; audio must never show explosion Firing or Track Players. Supported targets must retain their own settings. Triggers must reset their setup step, and standalone teleporters must keep their dedicated editor.
5. Select an audio emitter and the barricade together, in both selection orders. No Bifrost audio/explosion setting should be offered to the mixed selection. Repeat with a compatible pair, which should retain shared controls, including mixed-value indicators when values differ. Remove one selected target and reopen to verify the list is rebuilt.
6. Change one supported value and Apply/Close, reopen and check persistence. Change it again and Cancel; check that the saved value remains. Switch to an incompatible target and confirm it received no value or extra control. Repeat with trigger conditional options and a mixed selection.
7. Close Properties through its close button, outside click and Escape where supported. Repeat with an option dropdown open; switch modes or exit GM while open. Reopen several times and confirm there is no invisible catcher, old dropdown, stale category, retained scroll callback or scenario-only control.
8. After each close, immediately test normal selection, Ctrl-add/remove selection, box selection, drag/move, resize and right-click actions. Close or interrupt a menu while a mouse button is held, then release outside it; the first normal action afterward must work without click-through or a stuck gesture.
9. Repeat the selection/filter/apply/cancel cases with a remote GM on a dedicated server. Have a second client observe a supported gameplay change, reconnect and join late, and verify the resulting state. Confirm an unauthorized client cannot apply GM changes. Record listen-server, dedicated, remote and JIP outcomes separately.

The code changes are saved and compilation is verified. Visual/input/multiplayer acceptance and the clean-session check above are required before declaring the edge case fully closed.
