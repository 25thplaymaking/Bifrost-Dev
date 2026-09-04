# GM Startup Crash Decision Record — 2026-09-01

## REQUIREMENTS

- Diagnose the crash that occurs after entering Game Master from the latest Workbench session.
- Keep this startup failure separate from Vehicle Service unless runtime evidence connects them.
- Remove the project-owned renderer error immediately preceding the DirectX device hang.
- Preserve all unrelated working-tree changes.

## MINIMUM COMPONENTS NEEDED

- The latest Workbench console, crash, and dump timestamps.
- The project-owned `DCO_GMPanel.layout` resource loaded immediately before the failure.
- One valid static icon resource already proven by another widget in the same layout.

## REJECTED/NEEDS CLARIFICATION BEFORE ACTION

- No Vehicle Service lifecycle changes: its menu and preview environment were not entered during this crash.
- No graphics-setting, driver, or hardware changes: the log identifies a project-owned invalid widget image first.
- No broad GM panel redesign.

## PRIMARY RISKS

- A DirectX device hang is engine-level; removing the preceding invalid resource use eliminates the actionable project defect but still requires a fresh runtime test to prove causality.
- The working tree contains substantial unrelated work that must remain untouched.

## REQUEST INTERPRETATION

Inspect the newest crash evidence, correct the narrow Bifrost fault supported by that evidence, and return a single GM-entry retest boundary.

## UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY

Restore stable entry into Game Master without conflating this renderer crash with the separately tracked Vehicle Service interaction and reuse work.
