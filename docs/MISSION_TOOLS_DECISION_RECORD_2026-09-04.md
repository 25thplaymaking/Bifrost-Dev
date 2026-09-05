# Mission tools selection and decision record

REQUIREMENTS
- Review the supplied shared interaction and implement useful Arma 3 mission-making outcomes as original Reforger features.
- Selected: P0 Hide Terrain Objects; A1 Scale Object; A2 Create/Edit Intel; A3 Global Hint; A4 Chatter; A5 Make Invincible; A6 Create LZ, Create RP, Create Target; A8 Create Teleporter.
- Preserve exact feature names, existing uncommitted work, Bifrost presentation, server authority, remote-player delivery and join-in-progress reconstruction.
- Record native-contract evidence, implementation actions and verification boundaries. Do not publish.

MINIMUM COMPONENTS NEEDED
- Existing context menu and a single Bifrost mission-tool editor for authored text and bounded settings.
- Replicated terrain-area and player-interaction helpers, plus existing editable-entity state and PlayerController request routes.
- Existing marker registry as the authoritative named-position catalog; native journal and teleport paths for player outcomes.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- A7 Damage Buildings is deferred after inspection: SCR_DestructibleBuildingComponent replicates collapse and its destroyed flag, but provides no symmetric restoration path. Setting health cannot honestly satisfy restoration of collapsed geometry, debris and navigation.
- B1-B10 remain conditional and are not selected. Arbitrary execution and dependency-bound Arma 3 functionality remain excluded.

PRIMARY RISKS
- Native editable transforms explicitly preserve local scale, so server SetScale alone does not establish remote/JIP scale.
- Hidden terrain needs collision handling, overlapping-area restoration and streaming reconstruction, not just local visibility flags.
- Intel content must only reach its authorized discovery audience; helper existence is separate from secret contents.
- Teleport must validate the requesting player, source proximity, destination, occupancy and cooldown on authority.

REQUEST INTERPRETATION
- The current request delegates feature selection and authorizes implementation; the shared conversation's earlier request to wait for feature IDs is historical context.
- Session persistence means across player reconnect/JIP within the running mission. Cross-mission composition storage remains its existing separate feature.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
Add useful, discoverable mission-making tools using Reforger's native systems and Bifrost's existing surfaces, with explicit multiplayer evidence and no claims beyond completed verification.

## Initial evidence
- Shared interaction: https://chatgpt.com/s/cx_6a9b1e6b6ee88191a763c85e02646b3d, final message shortlist P0/A1-A8/B1-B10.
- Enfusion MCP replication knowledge followed by the full Enfusion Engine replication overview in its documentation index, then PAC1CLI reads of installed data007.pak. The full BI Community Wiki Multiplayer Scripting page was also checked through the local index during final review.
- PAC1CLI confirmed editable transforms retain local scale; building collapse broadcasts and serializes m_bDestroyed without a repair method; native journal entries are local presentation objects populated by the map journal UI.
- Workbench WORKBENCH baseline: zero errors, 14 existing base-game warnings. Native validation works without injecting temporary MCP handlers.

## Final outcome
- Selected implementation, file/action ledger, operational limits and runtime acceptance are recorded in [MISSION_TOOLS_IMPLEMENTATION_2026-09-04.md](MISSION_TOOLS_IMPLEMENTATION_2026-09-04.md).
- Final compile/reload and post-cleanup validation: zero errors, unchanged 14 base-game warnings. Hidden native layout instantiation: 16/16 required widgets. Temporary handlers are absent.
- Dedicated-server, remote-client, listen-host, JIP and hands-on interaction evidence remain separate, uncompleted verification levels. No publication was requested or performed.
