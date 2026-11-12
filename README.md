# Oot-Terraform

Terraforming for [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) (the Ocarina of Time PC port):
build pieces, add water and reshape the ground of any area, from a menu inside the game.

Copyright © 2026 Grimsfrost. All rights reserved. See `AI-POLICY.md`.

This repository is a complete copy of Ship of Harkinian with the terraforming mod built in. It contains no game data
and no Nintendo assets: the game reads those from your own ROM, as Ship of Harkinian always does.

**Status: experimental.** Sculpting the ground in particular has had little testing.

## What it does

Hold **L** and press **R** in the game to open the Terraform menu. The world stops and the camera rises to an overhead
view.

- **Pieces:** stairs, ramp, platform, wall, pillar and bridge, each with real collision.
- **Water pool:** a block of real water to wade, swim and dive in.
- **Sculpt the ground:** a brush that raises and lowers the area's own ground.
- **Remove the nearest piece**, **Clear everything built in this area**, **Put this area's ground back as it was**.

Everything is saved per area with the save file and is there again when you return.

### Controls

Menu: stick or D-pad up/down to choose, **A** to pick, **B** (or L+R again) to close. The mouse works too.

| Placing a piece | |
|---|---|
| Stick | Move |
| C-Left / C-Right | Turn |
| C-Up / C-Down | Raise or lower (hold R: taller or shorter) |
| D-pad up / down | Longer or shorter |
| D-pad left / right | Wider or narrower |
| L | Next texture (18 from the game: stone, brick, wood, rock, ice, metal, grass...) |
| Z | Drop onto the floor below |
| A | Set down |
| B | Cancel |

| Sculpting | |
|---|---|
| Stick | Move the brush |
| A (hold) | Raise |
| Z (hold) | Lower |
| D-pad | Brush size |
| B | Done |

### Limits

- At most 16 pieces per area.
- Water pools can't be turned: the game's water lies square to the world.
- Stairs look like steps but are walked as a smooth slope.
- Sculpting moves the ground, not the things on it (trees, signs, doors, enemies).
- After a large raise or dig, leave the area and come back so the game re-sorts its ground.
- Indoors, sculpting can catch a floor above or below (it reaches about 300 units up and down).

## Installing

1. Download the zip for your system from **Releases** (Windows, or Linux x86_64 / ARM) and extract it to a folder.
2. Start `soh.exe` (or the AppImage on Linux) and point it to your own legally dumped Ocarina of Time ROM, exactly as with
   Ship of Harkinian.
3. Load a save and hold **L**, then press **R**.

To turn the menu off, set the console variable `gEnhancements.Terraform.Enabled` to `0`.

### Building it yourself

The mod lives in `soh/soh/Enhancements/Terraform`, plus four small hooks: two in `soh/soh/z_scene_otr.cpp`
(`Terraform_BeforeCollision` before the collision is allocated, `Terraform_RoomLoading` as a room loads) and two in
`soh/soh/SohGui/SohGui.cpp` (the Terraform window is created and destroyed with the others). Build it as you would
Ship of Harkinian; every push here also builds it on GitHub (Actions > generate-builds).

## How it works

- **Pieces** are actors with dynamic collision built in code (a box and a wedge), drawn as plain lit stone.
- **Water pools** are added to a copy of the area's water list while you are in the area.
- **Sculpting** keeps a grid of height offsets per area and applies it to the area's collision points and to the
  vertices of its room meshes. The game's files are never changed: the originals are put back in memory when you leave
  the area.
