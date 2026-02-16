# B-RUSH

**Real-time tactical drawing overlay for VALORANT.**

Draw on your screen. Your teammates see it. Coordinate better. Win more.

B-RUSH connects players through a lightweight WebSocket relay (`ws.b-rush.it`), syncing drawings across every client on
your team in real time.

---

https://github.com/user-attachments/assets/e07a70d8-58d4-4fe4-8b38-dfe2521f8901

## ⚠️ Early Access

This is a **testing release** — the goal is to catch crashes, lag, and edge cases before a wider rollout.

- **Do not use on your main account or in ranked.**
- Supports display scaling, but **not** non-standard aspect ratios (yet).

---

## Is It Safe?

The module **does not read game memory** (yet — see roadmap). All binaries are **code-signed**. The overlay is purely
visual and communicates only with the b-rush relay server.

That said, use at your own risk. It's early.

---

## Getting Started

0. Grab the latest files from [/releases](https://github.com/asmxes/b-rush/releases)
1. Launch **VALORANT**
2. Run **b-rush.exe** (or `injector.exe chalkboard.dll` manually)
3. Wait for the `b-rush vX.X.X.X` watermark on the top-left corner

### Controls

| Key                | Action                    |
|--------------------|---------------------------|
| `CAPS LOCK` (hold) | Show drawings             |
| `RIGHT MOUSE`      | Draw (while CAPS held)    |
| `E`                | Erase your drawings       |
| `CTRL + E`         | Clear everyone's drawings |
| `DELETE`           | Unload b-rush             |

> Custom hotkeys are planned but not available yet.

---

## Components

| File             | Description                                                         |
|------------------|---------------------------------------------------------------------|
| `b-rush.exe`     | Loader — handles auto-injection and updates. **Not available yet.** |
| `injector.exe`   | Injects the main module into the game process.                      |
| `chalkboard.dll` | The core — drawing, networking, overlay rendering.                  |
| `.config`        | Configuration file. Place in `%APPDATA%\b-rush\`                    |

---

## Configuration

Place `.config` in `%APPDATA%\b-rush\`. Format is `KEY=VALUE`.

| Key             | Default              | Description                                                      |
|-----------------|----------------------|------------------------------------------------------------------|
| `LOG_LEVEL`     | `INFO`               | Log verbosity. Change to `TRACE` for debugging.                  |
| `WS_URI`        | `wss://ws.b-rush.it` | Relay server. Only change if self-hosting.                       |
| `ROOM_OVERRIDE` | *(empty)*            | Force a specific room. Only matched players will share drawings. |

---

## Roadmap

**High Priority**

- Rotate drawings with the minimap (currently requires Fixed Minimap in game settings) (Requires memory reading)
- Auto-clear drawings on new round (Requires memory reading)
- Auto-hide drawings during combat phase (Requires memory reading)
- Display agent icons with assigned team colors
- Toggle visibility of individual player's drawings (Requires memory reading) (RawInputDataBuffer hook?)
- Unit tests

**Medium Priority**

- Configurable hotkeys
- Loader module with auto-install and auto-update

**Low Priority**

- Rewrite the relay server *(it's vibecoded — if it works, it works)*

> Items marked with memory reading depend on compatibility with Vanguard.

---

## Something Wrong?

Enable `TRACE` logging in `.config` if you're debugging.

Otherwise, reach out:

- Discord: **@asmxes**
- Server: [discord.gg/nUtzkzng](https://discord.gg/nUtzkzng)

---

## Contributing

Branch from `develop`:

- `feature-xxx` for new stuff
- `fix-xxx` for bug fixes

Open a MR when ready.

---

## Credits

- [Dear ImGui](https://github.com/ocornut/imgui) — overlay rendering
- [Reahly](https://github.com/Reahly) — Riot local API implementation
- Claude — vibecoded scripts that somehow work :D
