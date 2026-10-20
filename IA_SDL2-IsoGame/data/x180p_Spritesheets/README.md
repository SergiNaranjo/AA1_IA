# Character art credit

Source: **[Animated Isometric Prototyping Hero](https://engvee.itch.io/animated-isometric-prototyping-hero)**
by [engvee](https://engvee.itch.io/) on itch.io. Free, for prototyping /
non-commercial use — see the itch.io page for the exact license terms.

## What this folder is

This is the unmodified "x180p" (180px frame size) spritesheet download from
that page, unzipped directly into this directory — one subfolder per
animation, e.g. `WalkForward_Unarmed/`, `Attack_Sword/`. Nothing here has
been renamed, recropped, or repacked; the engine reads the files exactly as
they come out of the zip.

`engine/Game.cpp` builds each file path from the animation name, weapon and
direction angle, e.g.
`data/x180p_Spritesheets/WalkForward_Unarmed/WalkForward_Unarmed_Body_000.png`.

## Which animations the game uses

Each animation set has 16 directions (`_000` .. `_337`, 22.5-degree steps)
and a separate `_Body_`/`_Shadow_` layer pair per direction.

- **Used by the demo:** `Idle_<Weapon>`, `WalkForward_<Weapon>`, `Attack_<Weapon>`
  for `Weapon` = `Unarmed`, `Sword`, `Bow` (no `Attack_Unarmed` — the pack
  doesn't include one).
- **Included but not used yet:** `Death_*`, `Jump_*`, `Run_*`,
  `Draw_*`, `Hit_*`, `StrafeLeft_*`, `StrafeRight_*`, `WalkBack_*` — left in
  place so you can add them (e.g. a `Death` animation when an agent is
  defeated).
