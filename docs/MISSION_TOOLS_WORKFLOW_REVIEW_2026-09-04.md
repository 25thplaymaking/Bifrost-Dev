# Mission tool workflow review

REQUIREMENTS
- Review all 13 mission actions against the actual Arma 3 ZEN/Achilles workflows, including placement, configuration, editing, removal and multiplayer authority.
- Correct the reported Hide Terrain Objects workflow: place a selectable system, show its radius, edit its size, hide scenery and restore it when disabled, moved, shrunk or deleted.
- Diagnose the shared world-click failure and correct confirmed defects without copying Arma 3 source.
- Keep inputs labeled and report remaining parity gaps and untested runtime behavior explicitly.

MINIMUM COMPONENTS NEEDED
- Existing terrain helper becomes a native editable system with native properties and catalog registration.
- Existing terrain state tracks changes and releases obsolete hide claims.
- Existing targeting bridge and mission controllers receive narrow corrections.
- This review records each action's verified implementation and differences.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No replacement editor framework, new dependencies, release, or production restart.
- Matching a feature name or compiling a script is not proof of Arma 3 workflow parity.

PRIMARY RISKS
- Native input contexts differ between placing a prefab and selecting a world target.
- Terrain meshes may be generic entities rather than Building/Tree classes.
- Overlapping hide areas must retain one another's claims when resized or removed.
- Dedicated-server, remote-client and JIP behavior require separate evidence.

REQUEST INTERPRETATION
- Audit every newly exposed function and repair confirmed defects, with the user's placeable, editable hide-area behavior as the immediate acceptance case.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Replace misleading setup-only behavior with a usable native hide system, and give an evidence-backed account of how every mission action compares with Arma 3.

## Follow-up decision: marker refresh exception

REQUIREMENTS
- Fix the reported RefreshPositions null reference and inspect sibling marker refresh, empty-list and delayed-reply paths. Initial investigation uses source and MCP; the user subsequently authorized computer use for validation while away.
- Preserve a named selection across replacement snapshots and keep authoritative validation on the server.
- Validate the final scripts through the native MCP compiler; distinguish source checks from runtime reproduction.

MINIMUM COMPONENTS NEEDED
- The existing mission panel owns copies of its displayed marker records, using the marker record's existing Copy method.
- Narrow guards and reply correlation in existing controllers where the lifecycle sweep demonstrates a defect.
- An isolated test probe exercises production snapshot and reply methods without modifying the scenario. Its retained source lives outside shipped script modules.
- This document records findings and completed evidence; no new service or UI framework.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No new application process, publishing or claim of multiplayer acceptance from compilation.

PRIMARY RISKS
- The marker service destroys the old snapshot before notifying subscribers; weak UI references therefore become null.
- A delayed reply can reach a panel that now displays a different action or target.

REQUEST INTERPRETATION
- Complete the pending repairs using files and Enfusion MCP, then use the newly authorized desktop access to repeat the failing user workflow.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Make repeated marker creation, refresh and mission-tool configuration safe and verify the reported sequence in the local engine.

## Findings for every mission action

This is a workflow comparison, not a declaration of full Arma 3 parity. Native Reforger equivalents remain subject to separate multiplayer acceptance. The checked Arma 3 references are ZEN commit `b5786646a397593831232675a138cd6e35924356` and Achilles commit `f123656459cab7766aa40c32d5ee12d29ebadaae`; their code was read to understand behavior, not copied.

| Action | Arma 3 behavior checked | Bifrost result and differences |
| --- | --- | --- |
| Hide Terrain Objects | ZEN offers a radius preview, hide/show mode and category filters; its temporary module is deleted when its dialog opens. The requested persistent editable circle is an additional requirement. | Repaired: a real native editable SYSTEM in Lightning > Bifrost, ground preview, persistent circle, native radius and on/off properties, movement and removal lifecycle. Radius is 5–100 m. Generic static mesh scenery is included alongside buildings and trees; editable entities, characters and vehicles are excluded. No separate scenery-category filters. Terrain height and baked navigation are unchanged. |
| Restore Hidden Terrain | ZEN's hide dialog has a Show mode for a chosen radius. | Bifrost's separate command restores all Bifrost hide areas. Individual restoration is now available through each system's Hide objects OFF property. This command is broader than ZEN's radius-based Show. |
| Scale Object | ZEN reads the attached object's current scale and provides a slider. | Absolute uniform scale on eligible standalone static editable props; existing single-target scale now prefills the text box. Range 0.25–4. Vehicles, buildings, characters, dynamic bodies and hierarchies remain excluded because the existing scale implementation does not establish safe behavior for them. |
| Make Invincible | ZEN changes damage handling and can include the vehicle's current crew. | Matches that core operation through server validation and replicated editable state. Does not repair damage or protect future occupants. Actual remote damage and reconnect tests remain outstanding. |
| Create/Edit Intel | ZEN can use an attached prop or create a chosen intel object, with text, discovery sharing and optional removal. | Native Read Intel and map-journal delivery, editable title/body, finder/faction/all-player audiences and optional clue removal. Repaired the incorrect reuse of scaling eligibility: props with children or movable physics can now carry interactions. Requires an existing prop; automatic clue creation, group-only sharing, custom hold duration and sound are absent. |
| Global Hint | ZEN authors and previews formatted hint text for current players. | Native global title/body hint delivery provides the core announcement. No rich-text preview. Transient by design. |
| Chatter | ZEN uses chat channels with a chosen AI/vehicle speaker or faction HQ. | Repaired delivery from hints to the native chat feed, visibly marked [AI]. Validates a selected AI is living and conscious; no selection uses the GM's faction HQ. Audiences are everyone, speaker faction or nearby 100 m. Vehicle commander extraction, group/command/vehicle channels and an arbitrary HQ faction picker are absent. |
| Create Teleporter | ZEN creates named endpoints on attached objects or a new flagpole and offers a destination menu across endpoints. | A native Travel interaction connects exactly two existing props by a shared link name. Server checks requester, proximity, line of sight, destination and cooldown. This is a paired shuttle, not ZEN's destination network. Automatic endpoint-prop creation and a multi-destination picker are absent. |
| Use Named Position | Adapter for named logic positions used by other systems. | Server applies a native Move waypoint to supported AI groups or moves a supported task/strike/mortar operating centre. It does not implement helicopter landing or a laser designator. Existing strike snapshots remain unchanged during their current pass. |
| Remove Intel / Teleporter | Removal depends on the source module/object workflow. | Bifrost explicitly removes the interaction helper and preserves its source prop. Removing one endpoint leaves its partner unpaired. This is an additional management action, not an Arma 3 module equivalent. |
| Create LZ | Achilles creates a named logic and an invisible AI helipad. | Currently creates a named position in the server marker library. There is no editable world logic or native landing pad. This is a material workflow gap, not full LZ module parity. |
| Create RP | Achilles creates a named reinforcement-point logic. | Currently creates a named marker-library position. There is no corresponding selectable world system; it can be consumed by Use Named Position. This is a material editing/discovery gap. |
| Create Target | ZEN creates a named target logic with optional faction laser target. | Currently creates a named marker-library position. There is no selectable world target logic or laser option. This is a material workflow gap. |

Primary reference implementations: [ZEN module functions](https://github.com/zen-mod/ZEN/tree/b5786646a397593831232675a138cd6e35924356/addons/modules/functions), [ZEN global hint editor](https://github.com/zen-mod/ZEN/blob/b5786646a397593831232675a138cd6e35924356/addons/modules/functions/fnc_gui_globalHint.sqf), and [Achilles reinforcement modules](https://github.com/ArmaAchilles/Achilles/tree/f123656459cab7766aa40c32d5ee12d29ebadaae/%40AresModAchillesExpansion/addons/modules_f_ares/Reinforcements/functions). The local reference clones were inspected for the exact function bodies. No claim is made that persistent world modules, paired destinations and marker-library coordinates are interchangeable.

## Shared input and authoring corrections

- Setup targeting listens to the native `EditorSetSelection` click action. Prefab placement continues through `EditorPlaceAndCancel`. A successfully placed QRF control on the same clear terrain establishes that the earlier fence attempt was not a valid failure test for either prefab.
- Targeted object configuration uses the clicked editable object instead of an older selection. Ground actions ignore object selection and explicitly report that the ground position was chosen.
- Targeting instructions remain in the CREATE footer until completion or cancellation, instead of a native hint hidden behind CREATE. Closing the targeting flow clears the stale instruction.
- Text fields retain bordered input areas, persistent captions, examples, limits and short numbered steps. Radius and Hide objects use native properties with persistent explanatory descriptions.
- A runtime exception exposed the enabled checkbox inheriting the radius list attribute. The checkbox now derives directly from the native base attribute; properties were reopened successfully after reload.

## Evidence from this local playtest

- Native script validation: zero errors, 14 base-game warnings. Native UI reload confirmed Game Scripts reloaded.
- At 18:35:57 local time, the server log records `E_DCO_TerrainArea.et` placed on clear ground at `<3022.67,71.0891,1575.89>`. The system is selectable and counts against SYSTEMS budget.
- Observed scenery disappear inside the 35 m orange circle. Opening Edit Properties succeeded with Radius and Hide objects controls. Switching Hide objects OFF restored rocks and vegetation and changed the circle to gray. Re-enabling at 15 m produced a smaller orange circle, with scenery outside it visible.
- At 18:37:47, the server log records the QRF `E_DCO_TaskZone.et` at `<3035.44,70.9048,1555.6>`. Its circle and stage-area controls appeared. Both requested placement checks succeeded on clear ground.
- Create LZ then accepted a terrain click and opened its bordered name field. This establishes shared ground targeting and rendered input visibility, not full LZ gameplay parity.
- An earlier script reload surfaced a native resource-leak assertion. Its cause was not isolated. The later native reload completed; this must not be represented as a proven Bifrost fix or a proven unrelated baseline issue.

These are local Workbench playtest observations. Dedicated server, remote clients, JIP/streaming, collision traversal, overlapping-area movement/removal and all remaining action outcomes are not proven by this run. Native prefab placement uses the editor's SYSTEMS budget; the legacy direct hide request's 16-area limit does not cap native placement.

## End-user hide workflow

1. Open CREATE > Lightning > Bifrost and select Hide Terrain Objects.
2. Click clear ground near the scenery. The system icon and its circle appear; scenery inside the circle disappears on the next update.
3. Double-click the system icon, or right-click it and choose Edit Properties.
4. Set Radius to the desired distance in metres. Close properties to apply it. Smaller radii restore scenery outside the new boundary unless another area still covers it.
5. Set Hide objects OFF to restore this area, or ON to hide it again. Move the system to relocate its area; removing it releases its claims. Restore Hidden Terrain removes every Bifrost hide area.

## Marker crash and related lifecycle repairs

The marker service clears its strong server-record array before notifying subscribers of a replacement snapshot. The mission panel previously retained weak entries and read the old selected record's `m_iId` after those entries became null. The panel now owns independent `Copy()` records in an `array<ref DCO_GMMarkerRecord>` and retains selection by ID. Rename and reorder preserve selection; deleting the chosen destination disables Apply until the GM deliberately chooses another.

The related reply review found that delayed operation results and private intel/teleporter settings could be applied to a different editor after closing, reopening or switching tools. Each request now carries a sequence through the server and owner reply. The panel accepts only its current pending request while open, and private settings additionally require the matching target and interaction tool. The server rejects attempts to edit or overwrite a prop using the other interaction purpose; the GM must remove its existing interaction first.

The sibling marker tree rebuilds before rendering its replacement snapshot, and composition refresh replaces its entries before drawing rows. Neither uses the mission panel's old-record-before-refresh pattern. No speculative changes were made to those paths.

Evidence:

- Consulted the MCP Enforce reference-lifetime documentation and the full BI Wiki [Automatic Reference Counting](https://community.bistudio.com/wiki/Arma_Reforger:Scripting:_Automatic_Reference_Counting) page. PAC1CLI inspection of installed native editor attributes confirmed strong array ownership and the separate checkbox/value-list attribute bases.
- Native script reload at 19:43:06 compiled production code and the isolated regression probe with zero errors and 14 base-game warnings. A prior test-module override error was corrected by placing the test probe in Game and its NET API handler in WorkbenchGame; production methods were not changed to accommodate the test.
- The local NET API regression returned **18 passed, zero failures**. Checks cover expired original weak entries, independent display copies, filtering, rename/reorder, deleted selection, repeated snapshots, empty paging, and stale/duplicate/closed/reopened request results.
- Fresh local GM playtest: CREATE displayed Bifrost 21 / FX 8, with a real Hide system and no composition entry. Terrain targeting opened the bordered and labeled position editor. Native keyboard entry worked. Two consecutive saves completed at **19:47:46.256** and **19:47:55.994**, and Close returned to GM with the saved world label visible. The reported null-reference exception did not recur.
- Evidence log: `logs/logs_2026-09-04_19-36-47/script.log` under the Workbench profile. Startup still recorded native slot/config and EditBoxFilter warnings; leaving play recorded an editor CLOSE/MODE_DELETE overlap warning. Their causes were not isolated, and this is not a claim of an entirely warning-free runtime.

The runtime probe is retained under `Tests/Workbench/` outside shipped script modules. Temporary Game and WorkbenchGame copies are absent by filesystem inspection. Final native WORKBENCH validation after cleanup passed with zero errors and 14 warnings. Workbench was left in edit mode with Play available and Save World disabled; the disposable playtest was not saved into the scenario. This establishes local regression and user-flow evidence; dedicated-server transport, remote clients and join-in-progress remain separate, unverified acceptance levels.
