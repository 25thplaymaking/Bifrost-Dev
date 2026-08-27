# Release validation

Bifrost releases use a compile-first validation policy. Real mod-stack compatibility is tested after a Workshop build is available because those combinations cannot be represented reliably before publication.

## Required before publishing

- Workbench script validation completes with zero Bifrost script errors.
- The current compile log contains no candidate-caused VM, GUI, replication, or resource errors.
- Static integrity checks find no missing resource metadata, duplicate GUIDs, merge debris, empty files, or unresolved editor attribute classes.
- The release version, changelog, README, source fingerprint, candidate commit, and addon identity agree.
- `Scripts/WorkbenchGame/EnfusionMCP` is absent and no Workbench process is left running.

## After publishing

Test the changed features with representative third-party mods and record reproducible failures for the next correction. Post-publication compatibility testing is not a blocker for publishing a compile-clean candidate.
