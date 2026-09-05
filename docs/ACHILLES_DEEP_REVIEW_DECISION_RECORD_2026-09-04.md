# Achilles deep review decision record

Date: 2026-09-04

```text
REQUIREMENTS
- Pin and deeply review the current default branch of ArmaAchilles/Achilles.
- Inventory every addon, registered function, documented module/action, execution path,
  locality boundary, and user-visible outcome.
- Compare every outcome against current Bifrost and installed Arma Reforger behavior.
- Implement every missing outcome that passes the complete portability gate.
- Preserve each newly ported feature's Achilles user-facing name.
- Record every material research, implementation, validation, and cleanup action.
- Keep gameplay mutations server-authoritative and disclose listen-server,
  dedicated-server, remote-client, and JIP evidence separately.
- Preserve the existing dirty Bifrost working tree and the completed ZEN review batch.

MINIMUM COMPONENTS NEEDED
- One pinned temporary research clone of Achilles.
- One source-led feature/function/registration inventory and crosswalk.
- Existing Bifrost native-action, Orders, context, marker, composition, trigger, FX,
  Arsenal, vehicle-service, world-control, and replication paths where outcomes fit.
- Only narrowly scoped original Enfusion additions for proven portable gaps.
- One exhaustive report, one action ledger, and one changelog entry.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- Do not copy Achilles SQF, configs, textures, layouts, sounds, or other assets.
- Do not import Arma 3, CBA, ACE, Ares, Zeus Enhanced, or other dependency code.
- Do not recreate unsafe arbitrary-code execution or engine-specific developer tooling.
- Do not claim a theoretical greenfield recreation is a direct portable feature.
- Do not add duplicate replacements for native Reforger actions Bifrost preserves.
- Do not release, commit, push, or publish without a separate request.

PRIMARY RISKS
- Achilles is an older Arma 3 extension whose dependency and module structure may
  overlap ZEN while using different names and locality assumptions.
- Name-only comparison can miss different outcomes or double-count inherited Ares work.
- Client-looking Zeus actions can mutate global Arma 3 state without an equivalent
  dedicated-server/JIP contract in Reforger.
- Broad implementation can duplicate current Bifrost or native Reforger features.
- The working tree contains substantial related user work that must remain intact.

REQUEST INTERPRETATION
- "Do the same" means repeat the completed ZEN standard: exhaustive pinned-source
  review, outcome-level Bifrost crosswalk, immediate implementation of complete safe
  gaps under original names, full action ledger, and explicit validation boundaries.
- "Portable" means a complete supported Reforger contract exists now, the feature can
  use current Bifrost architecture, has a server-authoritative multiplayer design, and
  needs no incompatible third-party dependency or copied content.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Audit Achilles end to end after the ZEN batch, identify what Bifrost already has,
  implement only genuine complete cross-engine ports, and leave an evidence-backed,
  reviewable record without overstating runtime multiplayer proof.
```
