# Authoritative Workspace Rule

All Bifrost implementation, fixes, diagnostics, tests and project documentation must be written directly in the existing workspace:

`C:\Users\Bryce\Documents\My Games\ArmaReforgerWorkbench\addons\Bifrost-Dev`

The authoritative project is this workspace's `addon.gproj`, with GUID `6A0C2D6CE9809C6E`, internal ID `BifrostDev`, and title `Bifrost-Dev`.

- Modify the existing implementation in place. Do not create separate fix projects, candidate copies, alternate checkouts or worktrees for this addon.
- Do not create nested or sibling pseudo projects, duplicate `addon.gproj` files, or copies of the addon identity inside the addon or its discovery paths. Duplicate GUIDs can cause the engine to load the wrong resources or scripts.
- Do not write Bifrost changes outside this workspace or redirect work into an external staging/release/fix folder. Any exception requires an explicit user instruction naming the alternate destination; convenience or isolation is not authorization.
- Add files only when required by the actual feature or test, within the existing project structure. Do not create parallel replacement implementations or standalone fix packages.
- Before authoring or validation, verify the absolute workspace path and `addon.gproj` identity. Before runtime validation, confirm Workbench's loaded path as well as the GUID. A matching GUID alone is insufficient because a duplicate checkout can share it.
- If Workbench is using another path, report the mismatch. Do not silently switch projects, copy changes between folders, or create another project to work around it.
- Preserve existing uncommitted workspace changes. Resolve overlaps in the authoritative files instead of moving the work elsewhere.
- Do not report changes as ready for testing unless they are saved in this workspace and validation evidence names this exact project path. Compilation does not replace the user's acceptance testing.

# Project Commenting Rule

Keep code comments concise and local to the code they explain.

Comments may state only:

- why the code is needed;
- what the code does when that is not already obvious.

Do not mention tools, investigation history, workflow, prompts, or implementation process in code comments. Remove commentary that merely restates the code.

# Multiplayer and Replication Rule

All project features and fixes must be ready for dedicated-server use. Client UI may select and configure, but authoritative gameplay changes must be validated and executed by the server, with shared state carried by replicated properties, native replicated systems, or reliable RPCs as appropriate. Treat listen-server, dedicated-server, remote-client, and join-in-progress evidence as separate verification levels; never infer one from another.
