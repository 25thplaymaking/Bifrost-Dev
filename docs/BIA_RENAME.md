# BIA code rename

REQUIREMENTS
- Rename every project-owned GRSA function and executable entry point to BIA.
- Update calls, constructors/destructors, RPCs, replication callback names, enums, menu presets and serialized component bindings together.
- Preserve existing saved kits/preferences, resource GUIDs and third-party provenance.
- Retain all candidate fixes and diagnostic hooks; validate and close Workbench afterwards.

MINIMUM COMPONENTS NEEDED
- Mechanical rename of existing code and affected text bindings.
- Existing serialized settings holders retain their legacy schema names; they contain data fields only, no GRSA functions.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No resource identity changes, binary asset rewrite, data deletion, new migration framework or publication.
- Historical review records retain historical names.

PRIMARY RISKS
- A stale reflection/RPC/layout string can compile but fail when invoked.
- Changing profile paths or console settings schema names could hide existing kits/preferences.
- Dedicated server and clients must use the same renamed build.

REQUEST INTERPRETATION
- BIA replaces GRSA in executable project code; legacy data-only names remain compatibility identifiers.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
Complete the function and code rename with matching bindings, keeping users' existing stored data accessible.

## Implementation and compatibility

- Renamed 42 script files and their executable identifiers from GRSA to BIA, including the Arsenal bridge.
- Updated class references, constructors/destructors, methods, RPC names, replication callbacks, menu presets, input actions, widget names and serialized layout/config bindings.
- Updated the layout verifier to discover BIA screen classes while retaining existing layout resource paths.
- Preserved resource filenames, GUIDs and metadata. Existing resource paths containing GRSA still resolve the same assets.
- Preserved the data-only GRSA_ArmorySettings and GRSA_ArmorySavedKit schema holders. Neither declares a GRSA function.
- Preserved legacy profile paths, the console settings module key, GRSAB1: kit envelope prefix and grsab2x marker so existing saved data remains readable.
- Historical review notes and third-party provenance remain intact.
- Existing bug-fix candidates and BF-DIAG instrumentation remain enabled.

## Validation on 2026-09-06

- Verified the live loaded addon is this candidate's addon.gproj, BifrostDev.
- Native WORKBENCH validation passed with zero errors and 14 base-game deprecation warnings, including after removing all temporary helpers.
- Layout verifier: 370 bindings across 26 layouts, zero failures, two fault-injection passes.
- Native menu/layout regression: 19 checks passed using the actual GM and Gunsmith layouts.
- Native session lifecycle regression: 34 checks passed, including studio lights and audio graph checks.
- Executable identifier sweep found no unexpected GRSA identifiers; only the two data-only compatibility types remain.
- Temporary Game probes and Workbench handlers were removed. Source changes are saved on disk. Workbench was closed; the final process inventory contained zero Workbench processes or children of the owned test process.
- Native validation log: logs_2026-09-06_12-37-05/script.log under the local Workbench logs directory.

## Acceptance boundary

These checks establish compilation, binding consistency and bounded local lifecycle behavior. They do not establish physical mouse behavior, rendered vest attachment correctness, dedicated-server/remote-client/JIP behavior, or user acceptance of the original bugs. All multiplayer participants must run the same renamed candidate. Nothing has been published or deployed by this rename.
