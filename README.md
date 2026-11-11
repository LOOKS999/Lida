# Lida

![build](https://github.com/LOOKS999/Lida/actions/workflows/build.yml/badge.svg)
![release](https://img.shields.io/github/v/release/LOOKS999/Lida)

A tiny Windows tray app that follows your system light/dark theme and swaps your
**wallpaper** and **cursor scheme** automatically.

Lida is **one executable**. No installer, no DLLs, no files to keep next to it. Run
`Lida.exe` and it creates its settings in `%APPDATA%\Lida\config.json`.

---

## Features

- Detects Windows light/dark mode through
  `HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\AppsUseLightTheme`
- A wallpaper and a cursor scheme for each mode (Light / Dark)
- Checks the theme on a configurable interval (default `1000` ms)
- Optional lock-screen wallpaper that follows the theme (needs administrator rights)
- Optional start at sign-in, elevated when possible (no UAC prompt at logon)
- Tray UI for everything — no file editing required
- Two interfaces: a **classic menu** or a **modern rounded panel** with hover rows and switches
- UI settings: light / dark / follow Windows / custom colors, accent, background, font and startup animation
- Custom embedded icon (resource 101) used by the tray, windows and Alt-Tab
- No network access, no telemetry

---

## Requirements

- Windows 10 or Windows 11
- Cursor schemes you want to use must already be installed
  (**Settings → Bluetooth & devices → Mouse → Additional mouse settings → Pointers**).
  Lida selects schemes by name; it does not install cursor files.

---

## Install

1. Download `Lida.exe` from [Releases](https://github.com/LOOKS999/Lida/releases/latest).
2. Put it anywhere you like — for example `%LOCALAPPDATA%\Programs\Lida\`.
3. Run it. Lida appears in the notification area.

That is the whole install. On first run Lida writes:

```
%APPDATA%\Lida\config.json
```

Nothing else is created next to the executable, so `Lida.exe` can be moved, copied or
deleted like any other portable app.

To start with Windows, right-click the tray icon and enable **Start with Windows**.

---

## Usage

Right-click (or left-click) the tray icon:

| Menu item | What it does |
|---|---|
| **Light Mode → Set Wallpaper...** | Pick the wallpaper used in light mode |
| **Light Mode → Set Cursor Scheme by Name...** | Type an installed scheme name, e.g. `Modest Cursors Light` |
| **Dark Mode → ...** | Same two options for dark mode |
| **Auto Switch on Theme Change** | Toggle automatic switching |
| **Apply Current Theme Now** | Re-apply the current mode immediately |
| **Restore Default Cursors** | Revert to the Windows default cursor scheme |
| **Lock Screen Follows Theme** | Toggle lock-screen sync; ticked whenever Lida is actually controlling the lock screen |
| **Start with Windows** | Launch Lida automatically at sign-in |
| **Restart as Administrator...** | Only shown when Lida is not elevated |
| **Reload Config** | Re-read `config.json` after manual edits |
| **UI Settings...** | Appearance window (classic vs modern, colors, font, animation) |
| **Open Config Folder** | Open `%APPDATA%\Lida` |
| **Exit** | Quit |

With the **modern panel** the same actions appear as a rounded popup: a header, grouped
rows, hover highlighting and switch toggles. Switch rows stay open so you can flip
several at once; action rows close the panel and run the action.

The scheme name must match a value under `HKCU\Control Panel\Cursors\Schemes`. List the
installed names with:

```
reg query "HKCU\Control Panel\Cursors\Schemes"
```

---

## Configuration

`%APPDATA%\Lida\config.json` is created on first run:

```json
{
  "check_interval_ms": 1000,
  "auto_switch": true,
  "lock_screen": false,
  "start_with_windows": true,
  "ui_style": "modern",
  "ui_theme": "system",
  "ui_bg": "#F8F7F4",
  "ui_accent": "#7F9C90",
  "ui_font": "Segoe UI",
  "ui_font_size": 10,
  "ui_anim": true,
  "light": {
    "wallpaper": "",
    "cursorScheme": ""
  },
  "dark": {
    "wallpaper": "",
    "cursorScheme": ""
  }
}
```

Notes:

- `check_interval_ms` — how often the theme is checked. Lower reacts faster, uses
  slightly more CPU. `1000` is a good default.
- `auto_switch` — `false` disables automatic switching.
- `lock_screen` — the desired state: `true` keeps the lock screen on the current mode's
  wallpaper, `false` turns the feature off. The tray toggle always shows the **real**
  registry state, not this value.
- `start_with_windows` — `false` stops launching at sign-in.
- `ui_style` — `"modern"` (rounded popup panel) or `"classic"` (native tray menu).
- `ui_theme` — `"light"`, `"dark"`, `"system"` (follow the Windows theme) or `"custom"`.
- `ui_bg` / `ui_accent` — colors as `#RRGGBB`. `ui_bg` is used when `ui_theme` is `"custom"`.
- `ui_font` / `ui_font_size` — font family and point size for the UI.
- `ui_anim` — `true` fades the panel in and shows the short startup splash.
- Wallpaper paths may be absolute or relative to the folder containing `Lida.exe`.
  The tray UI stores them relative when the file lives inside that folder.
- If a `config.json` sits next to `Lida.exe` (portable setup), it is used as the source
  and copied into `%APPDATA%\Lida` on first launch of a new copy.
- Restart Lida (or use **Reload Config**) after editing the file by hand.

---

## Interface settings

Open **UI Settings...** from either interface. Changes apply immediately and are saved to
`config.json`; there is no OK/Apply step.

| Setting | Options |
|---|---|
| **Tray interface** | `Classic menu` (native Windows menu) or `Modern panel` (custom rounded popup) |
| **Appearance** | Light, Dark, System (follows Windows), Custom colors |
| **Background** / **Accent** | Color pickers, enabled in Custom mode |
| **Font** | Any installed font + size (7–48 pt) |
| **Animate panel and splash** | Fades the panel in and shows a short centered "Lida" splash at startup |

The design goal is calm, not flashy: flat surfaces, a 16 px corner radius, one muted
accent, soft hover fills and small pill switches.

---

## How cursors are applied

Lida reads the selected scheme from `HKCU\Control Panel\Cursors\Schemes`, then writes
each of the 17 cursor roles (Arrow, Help, AppStarting, Wait, Crosshair, IBeam, NWPen, No,
SizeNS, SizeWE, SizeNWSE, SizeNESW, SizeAll, UpArrow, Hand, Pin, Person) into
`HKCU\Control Panel\Cursors`.

Scheme strings store paths as `%SYSTEMROOT%\Cursors\...`. Lida expands those environment
variables before writing, because the Windows cursor loader does not expand them itself —
the classic reason "the registry looks right but the pointer never changes".

After writing, Lida calls `SystemParametersInfoW(SPI_SETCURSORS, ...)` and broadcasts
`WM_SETTINGCHANGE` so running applications reload their cursors.

---

## Lock screen

When **Lock Screen Follows Theme** is on, Lida mirrors the active mode's wallpaper onto
the Windows lock screen by writing:

```
HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\PersonalizationCSP
    LockScreenImagePath     REG_SZ      <full path to image>
    LockScreenImageUrl      REG_SZ      <full path to image>
    LockScreenImageStatus   REG_DWORD   1
```

The tray toggle is not a stored guess: it is checked whenever that key is actually active
(`LockScreenImageStatus = 1` with a path set), so the menu and panel always show the real
state — including when the lock screen was set by an earlier run or by hand.

Turning the toggle **off** clears those values (`LockScreenImageStatus = 0`, path and URL
deleted) so Windows goes back to your own lock screen picture. Turning it **on** writes
the current mode's wallpaper immediately.

That key lives under `HKLM`, so both directions need administrator rights. If Lida is not
elevated, toggling asks whether to restart as administrator. Your choice is saved first,
so the elevated instance enforces it at startup (apply when on, clear when off).

Because a non-elevated app cannot write `HKLM`, the lock screen only updates while Lida
itself is elevated. To keep that true after a reboot, enable **Start with Windows** while
running as administrator — Lida then starts elevated at sign-in silently.

---

## Start with Windows

**Start with Windows** is on by default and registers:

```
HKCU\Software\Microsoft\Windows\CurrentVersion\Run
    Lida = "C:\path\to\Lida.exe"
```

When Lida is running as administrator, enabling the option instead creates a scheduled
task named `Lida` (logon trigger, highest privileges) and removes the `Run` entry. That
starts Lida elevated at sign-in with no UAC prompt, which is what makes the lock-screen
feature work after a reboot. Unticking the menu item removes both mechanisms.

---

## Build from source

You need Visual Studio 2022 with the **Desktop development with C++** workload (the Build
Tools are enough). From the repository root:

```
build.bat
```

The script locates `vcvars64.bat` automatically, compiles the icon resource, and produces
`Lida.exe` in the repository root. Intermediate files go to `build\`, which is ignored by
git.

Manual equivalent from an x64 Native Tools prompt:

```
rc /nologo /fo build\resource.res resource.rc
cl /nologo /std:c++17 /EHsc /W3 /O2 /DUNICODE /D_UNICODE /utf-8 /Fo:build\ /Fd:build\ ^
   src\main.cpp build\resource.res ^
   user32.lib gdi32.lib shell32.lib advapi32.lib comdlg32.lib ole32.lib ^
   /link /SUBSYSTEM:WINDOWS /OUT:Lida.exe
```

CMake is also supported (`CMakeLists.txt`). The icon is embedded from `icon.ico` as
resource ID **101** (`resource.rc`); keep that ID if you change the icon.

The GitHub Actions workflow in `.github/workflows/build.yml` builds the project on every
push and uploads `Lida.exe` as an artifact.

---

## Troubleshooting

**Cursor does not change**

1. Confirm the scheme name is exact: `reg query "HKCU\Control Panel\Cursors\Schemes"`
2. Confirm the values under `HKCU\Control Panel\Cursors` are fully expanded paths
   (e.g. `C:\WINDOWS\Cursors\...`), not `%SYSTEMROOT%\...`.
3. Confirm the `.cur` / `.ani` files exist on disk.
4. Sign out and back in once — Windows caches cursor sets per session in rare cases.

**Theme is not detected**

Check that this value exists and reads `0` (dark) or `1` (light):

```
reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize" /v AppsUseLightTheme
```

**Tray icon looks like a generic Windows icon**

The icon resource is loaded by numeric ID `101`. Make sure `resource.rc` contains
`#define IDI_ICON1 101` and rebuild.

**Lock screen does not change**

- Lida must be running as administrator (tray → **Restart as Administrator...**).
- The **Lock Screen Follows Theme** toggle shows whether the registry key is currently
  active; if it is unticked, Lida is not controlling the lock screen.
- Check `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\PersonalizationCSP` — if
  `LockScreenImageStatus` is not `1`, the write failed.
- Lock the PC once after changing the toggle.

**Lida does not start at sign-in**

- Tray menu → **Start with Windows** should be ticked.
- Check `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\Lida` or
  `schtasks /Query /TN Lida`.

**Wallpaper does not change**

Verify the path in `config.json` exists. Use the tray menu's file picker instead of typing
paths, so they are stored in the correct form.

**I want a clean start**

Exit Lida, then delete `%APPDATA%\Lida\config.json`. The next launch writes defaults.

---

## Repository layout

| Path | Purpose |
|---|---|
| `src/main.cpp` | Entire application source |
| `resource.rc` / `icon.ico` | Embedded icon resource (ID 101) |
| `build.bat` | One-step build script |
| `CMakeLists.txt` | Optional CMake build |
| `.github/workflows/build.yml` | CI build + artifact upload |
| `LICENSE` | MIT license |

Runtime files (not in the repo): `%APPDATA%\Lida\config.json`.

---

## Uninstall

1. Tray menu → **Start with Windows** (turn it off), then **Exit**.
2. Delete `Lida.exe`.
3. Delete `%APPDATA%\Lida`.

There is no installer and no registry footprint beyond the values Lida applied itself
(cursor scheme, autostart entry/task, and the lock-screen key if you enabled it). Use
**Restore Default Cursors** first if you want the stock pointers back, and pick your
wallpaper again in Windows Settings.

---

## License

MIT — see [LICENSE](LICENSE).
