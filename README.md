# Beneath - High Refresh Rate DeltaTime Fix

> Standalone native drag-and-drop fix for [Beneath](https://store.steampowered.com/app/1593230/Beneath/) that synchronizes Unity's physics timestep with your display's refresh rate and uncaps the framerate, eliminating camera judder, ghosting, and smearing.

[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%2F%2011%20x64-blue.svg)](#system-requirements--verified-environment)
[![Game](https://img.shields.io/badge/Game-Beneath%20(Steam)-orange.svg)](https://store.steampowered.com/app/1593230/Beneath/)
[![Installation](https://img.shields.io/badge/Installation-Drag%20%26%20Drop-green.svg)](#installation-guide)
[![License](https://img.shields.io/badge/License-MIT-lightgrey.svg)](LICENSE)

---

> [!IMPORTANT]
> ### Keep Files in the Game Directory
> **`dxgi.dll` and `BeneathDeltaTimeFix.ini` must reside side-by-side with `Beneath.exe` in the root game folder.**
>
> Windows will automatically load `dxgi.dll` upon launching `Beneath.exe`. No additional launchers, injectors, or mod loaders are needed.

---

## Features

- **Fluid Camera Motion:** Synchronizes Unity's internal `Time.fixedDeltaTime` with your monitor's exact refresh rate (e.g. 60Hz, 75Hz, 120Hz, 144Hz, 165Hz, 240Hz, 360Hz), completely resolving panning judder and micro-stuttering.
- **In-Memory Gravity Multiplier Scaling:** Fixes the game's internal physics flaw where changing `fixedDeltaTime` caused slow-motion "moon floating" when dropping off ledges. Falling speed is fully restored to vanilla velocity (5.88 m/s).
- **Physics Death Spiral Prevention:** Hard-limits physics catch-up steps per frame (`maximumDeltaTime = fixedDeltaTime * MaxPhysicsStepsPerFrame`), preventing CPU overload and permanent drops to ~9 FPS during intense scenes or stutters.
- **100% Drag-and-Drop:** Requires no external modding frameworks (BepInEx, MelonLoader) and no Special K.
- **Vanilla File Integrity:** Leaves all game files on disk (`Assembly-CSharp.dll`) completely original and unmodified.
- **Independent Rigidbody Physics:** Grenade throwing trajectories, enemy grenade throws, ragdolls, and environmental PhysX rigidbodies remain completely normal and timestep-independent.
- **Persistent Watchdog:** Continuously monitors and re-enforces your configured refresh rate across level changes, cutscenes, and player respawns.
- **Dynamic On-The-Fly Reload:** Reads `BeneathDeltaTimeFix.ini` in real time. Changes saved in Notepad are automatically detected and applied within one second without restarting the game.

---

## System Requirements & Verified Environment

| Requirement | Details |
|---|---|
| **Operating System** | Windows 10 / 11 (64-bit) |
| **Game** | [Beneath](https://store.steampowered.com/app/1593230/Beneath/) installed via Steam |
| **Verified System** | Windows 11 x64, 165 Hz display refresh rate |
| **Verified Game Build** | BuildID `26162641` (v1.05f, October 23th 2026) |
| **Dependencies** | None (100% self-contained native C proxy DLL, statically linked) |
| **Permissions** | Standard user permissions (no administrator rights needed) |

---

## Installation Guide

### Step 1: Download
Download the latest `Beneath-HighRefreshRate-Fix.zip` from the [Releases](https://github.com/EndzE/Beneath-HighRefreshRate-Fix/releases) page.

### Step 2: Extract to Game Directory
Extract both **`dxgi.dll`** and **`BeneathDeltaTimeFix.ini`** directly into your *Beneath* installation folder (where `Beneath.exe` is located):

```
...\steamapps\common\Beneath\
├── Beneath.exe
├── Beneath_Data\
├── MonoBleedingEdge\
├── dxgi.dll                 <-- Place here
└── BeneathDeltaTimeFix.ini  <-- Place here
```

> [!TIP]
> To find your game directory in Steam: right-click **Beneath** in your Steam Library -> **Manage** -> **Browse local files**.

### Step 3: Configure Refresh Rate
Open `BeneathDeltaTimeFix.ini` with any text editor (such as Notepad) and verify that `TargetHz` matches your display's refresh rate:

```ini
[Settings]
TargetHz=165
MaxPhysicsStepsPerFrame=2
UncapFramerate=1
Logging=1
```

### Step 4: Play
Launch **Beneath** through Steam or directly via `Beneath.exe`. Camera rotation and character movement will now be completely smooth.

---

## Settings & Configuration

Settings can be customized anytime in `BeneathDeltaTimeFix.ini`:

| Option | Type | Default | Description |
|---|:---:|:---:|---|
| **`TargetHz`** | `int` | `165` | Target refresh rate in Hz (e.g. 60, 75, 120, 144, 165, 240, 360). Sets `fixedDeltaTime = 1 / TargetHz` and scales falling gravity proportionally. |
| **`MaxPhysicsStepsPerFrame`** | `int` | `2` | Maximum physics simulation steps Unity is allowed to execute in a single frame (range: 1 to 4). Hard-limits `Time.maximumDeltaTime` to prevent physics accumulator death spirals. |
| **`UncapFramerate`** | `int` | `1` | `1` = Sets Unity's `Application.targetFrameRate = -1` (uncapped framerate). `0` = Leaves game default. |
| **`Logging`** | `int` | `1` | `1` = Writes diagnostic status and reload events to `BeneathDeltaTimeFix.log`. `0` = Completely silent. |

> [!NOTE]
> You can edit `BeneathDeltaTimeFix.ini` while playing. The watchdog checks the INI file once per second and applies the new timestep, physics limit, and gravity multiplier immediately without restarting.

---

## Technical Details

### 1. Camera Judder Root Cause
In vanilla *Beneath*, camera view rotation (`RotateView`) executes inside `Update()` at your monitor's full frame rate (e.g. 165 FPS), while character translation and physics step at Unity's default `Time.fixedDeltaTime = 0.02s` (50 Hz). On high refresh rate monitors, this mismatch causes the player's position to step 50 times per second while the camera pans at 165 times per second, producing severe judder, ghosting, and visual smearing.

The mod acts as a proxy for DirectX (`dxgi.dll`), attaches to Unity's embedded Mono runtime (`mono-2.0-bdwgc.dll`), and sets `Time.fixedDeltaTime = 1 / TargetHz` and `Application.targetFrameRate = -1`.

### 2. Falling Gravity Flaw & In-Memory Solution
When `fixedDeltaTime` is reduced, the game's movement controller (`FirstPersonController`) encounters an internal bug: inside `VisualTick()` (called every frame in `Update`), the developer placed `m_MoveDir.y = 0f;`, wiping out accumulated falling velocity every render frame. Consequently, falling velocity never builds momentum across frames, and the total distance fallen per second is:

$$\text{Fall Speed} = 9.81 \times 30 \times \text{fixedDeltaTime}$$

At 165 Hz (`0.00606s`), falling speed was reduced by 3.3x (from 5.88 m/s down to 1.78 m/s), causing the player to float in slow motion when dropping off ledges.

To resolve this without modifying disk files or affecting global PhysX gravity, `dxgi.dll` scans the in-memory IL bytecode of `FirstPersonController.PhysicsTick` upon launch and dynamically scales the player's gravity multiplier:

$$\text{Gravity Multiplier} = 30.0 \times \frac{\text{TargetHz}}{50.0} \quad (\text{e.g. } 30.0 \times 3.3 = 99.0\text{ at 165 Hz})$$

This mathematically restores the exact vanilla falling speed (5.88 m/s) while leaving all other physics (grenades, crates, ragdolls) at standard PhysX gravity (9.81 m/s²).

### 3. Physics Delta Time Death Spiral Prevention
In Unity, when a frame takes longer than normal (e.g., during scene streaming, shader compilation, or complex physics scenes with water/buoyancy), Unity accumulates elapsed time and runs multiple physics steps in a single frame to catch up, capped by `Time.maximumDeltaTime`.

If `maximumDeltaTime` is left at default values (0.05s to 0.10s) while `fixedDeltaTime` is 0.00606s (165 Hz), Unity attempts to execute **8 to 16 physics steps in a single visual frame**:

$$\frac{0.050\text{ s}}{0.00606\text{ s}} \approx 8.25\text{ steps}$$

If executing 8 to 16 physics steps takes ~80–90 ms of CPU time, the subsequent frame also takes ~110 ms, causing Unity to queue 8 to 16 steps again on the next frame. This traps the engine in a permanent loop known as the **Physics Delta Time Death Spiral**, locking the framerate to **~9 FPS**.

The mod completely eliminates this by strictly clamping `Time.maximumDeltaTime`:

$$\text{maximumDeltaTime} = \text{fixedDeltaTime} \times \text{MaxPhysicsStepsPerFrame} \quad (0.00606 \times 2 = 0.01212\text{ s at 165 Hz})$$

This mathematically guarantees that Unity can never execute more than 2 physics steps in any single frame. Even during severe performance hitches, CPU physics load remains minimal, and the game returns immediately to full refresh rate without ever collapsing into a death spiral.

---

## Building from Source

### Requirements
- MinGW-w64 GCC (x86_64)

### Compile
Run `build.bat` or compile via command line:
```cmd
gcc -O2 -shared -o dxgi.dll src/dxgi_proxy.c -static -static-libgcc
```

---

## Removal & Cleanup

To uninstall the mod completely:
1. Navigate to your *Beneath* installation folder.
2. Delete `dxgi.dll`, `BeneathDeltaTimeFix.ini`, and `BeneathDeltaTimeFix.log`.

---

## License

Distributed under the [MIT License](LICENSE).
