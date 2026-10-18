# Rules for editing this KiCad project (read first)

Original project folder (the ONLY place you may edit):
C:\Users\Chinm\Projects\DIY-Game-Controller\ELECTRONICS\coreBoard

1. Edit the existing project IN PLACE. Never copy, duplicate, or "save as" the project elsewhere.
2. Never call create_project, create_schematic, or any tool that creates a NEW project or board.
   Only open and modify the existing files in the folder above.
3. Every file path you pass to any tool (Konnect or Filesystem) must start with the folder above.
   If a path does not start with it, stop and ask me first.
4. Save all exports, Gerbers, BOMs, PDFs, snapshots and 3D models in:
   C:\Users\Chinm\Projects\DIY-Game-Controller\ELECTRONICS\coreBoard\output
5. Put any custom symbols or footprints you create in:
   C:\Users\Chinm\Projects\DIY-Game-Controller\ELECTRONICS\coreBoard\libraries
   Register those libraries at PROJECT scope (project library table), NOT global scope.
6. Do not modify KiCad's global settings or the standard KiCad libraries.
7. Before large edits, use snapshot_project (output goes to the output folder).
8. After every schematic edit, tell me to reopen the schematic in KiCad so it does not overwrite your changes.
9. At the end of each task, list every file you changed, with full paths.