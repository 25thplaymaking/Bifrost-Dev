REQUIREMENTS
- Review the current zen-mod/ZEN master source exhaustively enough to identify every shipped feature, its registration path, execution path, multiplayer locality, and observable outcome.
- Compare every identified ZEN feature with the current Bifrost implementation and classify it as existing, partially existing, portable, blocked by engine/content differences, or intentionally inapplicable.
- Implement every portable missing feature without dropping the existing Bifrost requests or weakening dedicated-server, remote-client, or join-in-progress behavior.
- Preserve the user-facing ZEN feature name for every ported feature.
- Record every repository action, source consulted, implementation decision, changed file, and verification result.
- Produce original Enfusion implementations informed by behavior, not copied SQF, UI resources, icons, or other GPL-covered implementation material.

MINIMUM COMPONENTS NEEDED
- A pinned source snapshot of ZEN and a generated source inventory grouped by addon/feature.
- A Bifrost capability inventory covering Game Master UI, context actions, editor attributes, placements, visualization, persistence, and replication.
- One durable parity matrix and action ledger under docs.
- Only the smallest Bifrost code/layout/config changes required for portable gaps confirmed by the matrix.
- Workbench compile/static validation followed by explicit dedicated-server, remote-client, and JIP test boundaries.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- Arma 3 engine patches, Eden-only behavior, ACRE/TFAR/ACE integrations, curator internals, and asset-dependent behavior are not assumed portable until an equivalent Reforger API and Bifrost use case are proven.
- ZEN source, artwork, layouts, localization, or other implementation assets will not be copied into Bifrost; GPLv3 obligations and the project's originality rule require an independently authored Enfusion implementation.
- A matching name does not justify a port when Bifrost already provides the same user outcome under that name or Reforger natively supersedes it.
- Release, Workshop publication, deployment, or commit creation are outside this request.

PRIMARY RISKS
- ZEN is an Arma 3 SQF/CBA/Zeus project while Bifrost targets Reforger Enfusion; superficially similar features may have incompatible authority, UI, or data models.
- A feature-by-folder count can miss behavior injected through config patches, event handlers, XEH, public functions, settings, and cross-addon calls.
- Porting a large list in one pass can create instruction-budget, UI-density, and replication regressions unless gaps are grouped around existing Bifrost systems.
- Static and Workbench validation cannot establish dedicated-server, remote-client, or JIP correctness.
- The Bifrost worktree contains pre-existing changes that must be preserved and distinguished from this review.

REQUEST INTERPRETATION
- “Every feature, every execution, how the outcome is made” means tracing each ZEN addon from configuration/registration through callable functions and network/local execution to its player-visible or GM-visible effect.
- “Does this exist here?” means comparing behavior and outcome, not merely matching class or menu names.
- “Portable” means the outcome can be implemented with current Reforger APIs, original Bifrost code, and a server-authoritative multiplayer design without importing ZEN code or assets.
- “Do it” authorizes implementing confirmed portable gaps in this working tree, but not publishing or releasing them.
- “Name the feature the same” means exact ZEN user-facing feature names for newly ported features, with Bifrost internals retaining the existing DCO naming convention where necessary.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Build an exhaustive, source-pinned ZEN-to-Bifrost feature map, then close every safe and useful portability gap with original, multiplayer-ready Enfusion work and a complete action ledger.
