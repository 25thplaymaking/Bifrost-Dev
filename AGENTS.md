# Project Commenting Rule

Keep code comments concise and local to the code they explain.

Comments may state only:

- why the code is needed;
- what the code does when that is not already obvious.

Do not mention tools, investigation history, workflow, prompts, or implementation process in code comments. Remove commentary that merely restates the code.

# Multiplayer and Replication Rule

All project features and fixes must be ready for dedicated-server use. Client UI may select and configure, but authoritative gameplay changes must be validated and executed by the server, with shared state carried by replicated properties, native replicated systems, or reliable RPCs as appropriate. Treat listen-server, dedicated-server, remote-client, and join-in-progress evidence as separate verification levels; never infer one from another.
