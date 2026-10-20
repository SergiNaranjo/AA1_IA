# IA_SDL2-IsoGame

An **isometric SDL2 sandbox for teaching Artificial Intelligence** in a video-game
development BSc. It gives students a clean, object-oriented world in which to
implement **steering behaviours**, **pathfinding** (A\*/Dijkstra), and **decision
making** (finite state machines, decision trees) — and watch them run on an
isometric map.

All AI runs in plain 2D world-space; the isometric projection is applied only at
draw time, so you never have to touch rendering or isometric maths.

This project began as Johan Forsblom's isometric SDL2 tutorial (Part 5), and was
rebuilt into an agent-based teaching framework by Lluis Gomez
(lluisgomez@enti.cat).

## Build & run

### Windows (Visual Studio) — recommended

Everything needed is included (SDL2 and SDL2_image are bundled in
`external/SDL2`), so there is nothing to install besides Visual Studio 2019 or
newer with the **"Desktop development with C++"** workload.

1. Unzip the project somewhere (don't open it from inside the zip).
2. Double-click **`IsoGame.sln`** (or *File > Open > Project/Solution*).
3. Make sure the toolbar shows **`Debug`** and **`x64`**.
4. Press **F5** (or *Ctrl+F5* to run without the debugger).

The executable goes to `bin\Debug\` (or `bin\Release\`), with `SDL2.dll` and
`SDL2_image.dll` copied next to it, and Visual Studio runs it from the project
folder so `data\` is found. The game opens **borderless fullscreen at your
screen's own resolution** — press `Esc` to quit, `F11` to drop to a window.
If it closes straight away, or you get a black screen, look in
`logs\runlog.txt`: it records the display, window size and graphics backend
before any of the game's own drawing happens. See
[Troubleshooting](#troubleshooting) below.

(Opening the folder with *File > Open > Folder* also works — it uses
`CMakeLists.txt` with the same bundled libraries — but then set the debug
working directory to the project folder yourself. The `.sln` is the simpler
route.)

### Linux / macOS

Install the SDL2 and SDL2_image development packages (e.g.
`sudo apt install libsdl2-dev libsdl2-image-dev`, or `brew install sdl2
sdl2_image`), then:

```sh
cmake -B build
cmake --build build
./build/isogame        # ESC or close window to quit
```

Run from the project root so that assets under `data/` are found.

### Controls (demo)

| Input | Action |
|-------|--------|
| `Left` / `Right` | Cycle which agent is selected |
| Left click | The *selected* agent walks to the clicked tile |
| `W` `A` `S` `D` | Scroll the map |
| Mouse near screen edge | Scroll the map |
| Mouse wheel | Zoom in / out (zooms around the screen centre) |
| `F1` or `I` | Toggle the debug overlay: each agent's target and the line to it, the tile each obstacle occupies, a marker on the selected agent, and the selected agent's influence map (`I` does the same as `F1`, for laptops where F1 is a brightness/media key) |
| `G` | Toggle the tile grid |
| `Tab` | Cycle the *selected* agent's weapon (Unarmed → Sword → Bow). In your own AI, a state machine or decision tree could make this choice instead. |
| `Space` | Play a one-shot attack on the *selected* agent (no-op while unarmed) |
| `F11` | Toggle borderless fullscreen / windowed |
| `Esc` | Quit |

The camera starts centred on the first agent and stops scrolling before you
lose sight of the map. `Left`/`Right` select an agent whether or not the debug
overlay is on; turn it on (`F1`) to see which agent is selected.

Agents can't walk through obstacles: when they bump into one, they slide along
its edge.

### Troubleshooting

**First stop: `logs\runlog.txt`.** It is written line by line as the game
starts, so it survives a crash or a hang. The first few lines report the SDL
version, the graphics driver, how many displays there are, the display's
current mode, the window size the game actually got, and which rendering
backend was chosen — then one line per sprite sheet as it loads.

| Symptom | What it means | What to do |
|---------|---------------|------------|
| Black screen, can't close it | The window is up but something is wrong with the display path | **Shift+F5** in Visual Studio (Debug > Stop Debugging) always kills it. Then try `F11`, or start with `--windowed` (below). |
| Window opens then closes immediately | An init step failed | Read the last line of `logs\runlog.txt` — it names the failure. |
| Game runs but there are no characters | Sprite sheets didn't load | Check for `WARNING: Texture:... was NOT loaded` lines. Usually the working directory (see next row) or, rarely, not enough graphics memory. |
| No `logs\runlog.txt` at all, or no art and an empty map | The game is running from the wrong folder | Run it from the project folder, not from inside `bin\Debug\`. The `.sln` sets this up already; `File > Open > Folder` does not. The game now prints this explicitly on startup when it happens. |
| Very low framerate | No hardware acceleration available | The log says `Render backend:software`. Update the graphics driver; the game still runs. |

**Starting windowed.** Pass `--windowed` (or `-w`) on the command line to boot
into a 1360×768 window instead of fullscreen. In Visual Studio: *Debug >
isogame Debug Properties > Debugging > Command Arguments*. `F11` does the same
thing while the game is running.

**If fullscreen still misbehaves on one particular machine,** these
environment variables let you test a different graphics path without touching
the code (set them under *Debug > isogame Debug Properties > Debugging >
Environment*, as `NAME=VALUE`):

| Variable | Try | Why |
|----------|-----|-----|
| `SDL_RENDER_DRIVER` | `opengl`, or `software` | Skips Direct3D, which is where most driver-specific trouble lives |
| `SDL_WINDOWS_DPI_AWARENESS` | `unaware`, `system`, `permonitorv2` | Overrides how the game handles Windows display scaling (it asks for `permonitorv2` by default) |

If one of those fixes it, say which machine and which setting — that's the
useful bug report.

**Why this mattered.** Earlier versions asked the monitor to switch to a
1360×768 video mode (SDL's "real" exclusive fullscreen). Lots of laptop
panels, classroom monitors and projectors don't have that mode, and when the
driver accepted the switch anyway you got a black screen — and because the
window never received keyboard focus, `Esc` never reached the game and it
couldn't be closed. The game now opens a borderless window at whatever
resolution the display is already running, so there is no mode switch to fail.
`WINDOW_WIDTH`/`WINDOW_HEIGHT` in `core/renderer.hpp` are set at runtime from
the real window size — read them, don't assume 1360×768.

### Maps

Levels are data-driven text files under `data/maps/` (see `data/maps/demo.map`):

```
size 64 64        # map size in tiles
tree 20 20        # tall billboard tree at tile (20, 20)
block 36 30       # cube block (tile-set tile 2) at tile (36, 30)
tile 5 40 40      # any tile-set tile as an object (5 = fence)
wall 42 26        # tall stacked-cube wall at tile (42, 26) — occludes correctly
repeat 5 0 1 wall 42 26   # 5 copies of the directive, stepping (dx, dy) tiles each time
```

All of these are obstacles: each one marks its tile as not walkable
(`World::isWalkable`), and agents can't walk into it. `block` and `tile` are
one-tile sprites from `isotiles.png`; `tree` is a taller sprite; `wall` is a
stack of cubes tall enough to hide an agent standing behind it. The map the
demo loads is chosen in `main.cpp`.

### Character art

The agent's animations come from a free third-party asset pack, credited in
[`data/x180p_Spritesheets/README.md`](data/x180p_Spritesheets/README.md).

### Influence maps

Each `Agent` has its own grid of numbers, one per tile, that your AI code can
write and read — use it for influence maps, cost fields, visited-tile heatmaps,
and so on:

- `initInfluence(world)` — call once, sizes the grid to the map
- `setInfluence(x, y, value)` / `influenceAt(x, y)` — write / read one tile
- `clearInfluence()` — reset every tile to 0

Each agent has its own grid, so (for example) two teams can keep separate maps.
In the demo, `main.cpp` fills each agent's grid with a simple gradient centred
on where it spawns, just to show what it looks like. Press `F1` to see the
selected agent's grid drawn over the map (warm colours = positive, cool =
negative), and `Left`/`Right` to switch agents.

## Code map & reading guide

Every `.hpp`/`.cpp` file starts with a short comment saying what the file is
(**FILE**), how it connects to the rest (**FITS IN**) and where to add your
own code (**EXTEND**). Functions you're likely to read or change have a comment
with the same parts: **Summary**, **Inputs**, **Output** (or side effects),
**Errors** (usually: a line in `logs/runlog.txt` and a safe fallback — the code
doesn't throw exceptions) and **Extend**. Three tags are worth searching for
(*Edit > Find and Replace > Find in Files* in Visual Studio):

| Tag | Meaning |
|-----|---------|
| `IMPORTANT INVARIANT:` | A rule you must follow (usually "call this before that"). Breaking it may not crash — things just quietly go wrong. |
| `WATCH OUT:` | Something that doesn't behave the way you'd guess. |
| `EXTEND:` | A good place to add your own code — e.g. a new case in a `switch`. |

**Where things live, in the order you'll probably want to read them:**

| Layer | Files | You'll edit this if... |
|-------|-------|-------------------------|
| Scene setup | `main.cpp` | ...you're placing/configuring agents, or choosing a map. |
| AI model | `engine/Vector2.hpp`, `engine/Agent.*`, `engine/World.*`, `engine/WorldObject.hpp` | ...you're implementing steering, pathfinding, or decision-making. This layer has no SDL or drawing code in it at all. |
| Map data | `engine/MapLoader.*` | ...you want a new `.map` directive or object kind. |
| Game loop & drawing | `engine/Game.*`, `engine/Animation.*`, `engine/DebugDraw.*` | ...you're adding a debug drawing, a key binding, or a new sprite animation. |
| Isometric renderer | `iso/isoEngine.*`, `iso/isoMap.*` | ...probably never. Camera and isometric projection maths, used only by `Game.cpp`. |
| SDL utilities | `core/*` | ...probably never. Window, renderer, textures, timer and logging. |

**What happens each frame:** `Game::run()` calls

1. `processInput()` — keyboard and mouse (key bindings, click to set a target);
2. `update()` — scrolls the camera, then `World::update` calls `Agent::update`
   on every agent, all in flat 2D world coordinates;
3. `render()` — converts world positions to screen pixels with
   `Game::worldToScreen` and draws everything.

The one rule worth keeping as you add code: AI code (`Agent`, `World`) never
deals with screen coordinates, and drawing code never makes AI decisions.
