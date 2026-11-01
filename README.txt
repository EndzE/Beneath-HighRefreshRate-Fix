Beneath - High Refresh Rate DeltaTime Fix
============================================

Game:    Beneath (https://store.steampowered.com/app/1593230/Beneath/)
Author:  EndzE (https://github.com/EndzE/Beneath-HighRefreshRate-Fix)
License: MIT License

DESCRIPTION:
Fixes camera judder, ghosting, and smearing on high refresh rate monitors
(e.g. 60Hz, 75Hz, 120Hz, 144Hz, 165Hz, 240Hz, 360Hz) by synchronizing Unity's
physics fixedDeltaTime with your monitor's display refresh rate and uncapping
the framerate.

VERIFIED ENVIRONMENT:
- Operating System: Windows 11 (64-bit)
- Display: 165 Hz refresh rate
- Game Version: BuildID 26162641 (v1.05f, October 23th 2026)

FEATURES:
- 100% Drag-and-Drop: No mod loaders (BepInEx / MelonLoader) and no Special K required.
- Leaves game files completely untouched (vanilla Assembly-CSharp.dll).
- Gravity Multiplier Scaling: Fixes the game's internal physics flaw where higher
  tickrates caused slow-motion "moon floating" when dropping off ledges.
- Preserves standard PhysX physics for grenades, rigidbodies, and props.
- Automatic Watchdog: Keeps your target refresh rate locked across scene changes,
  level transitions, cutscenes, and deaths.
- Dynamic Config: Supports any refresh rate via BeneathDeltaTimeFix.ini (updates
  on the fly within 1 second even while playing).

INSTALLATION:
1. Extract or copy "dxgi.dll" and "BeneathDeltaTimeFix.ini" into your Beneath
   game directory (next to Beneath.exe).
   Default Steam path:
   C:\Program Files (x86)\Steam\steamapps\common\Beneath\
2. Open "BeneathDeltaTimeFix.ini" with Notepad to set your monitor's refresh
   rate (e.g. TargetHz=144, 165, 240). Default is 165.
3. Launch the game!

CONFIGURATION (BeneathDeltaTimeFix.ini):
[Settings]
TargetHz=165        ; Set to your monitor's refresh rate (e.g. 60, 75, 120, 144, 165, 240, 360)
UncapFramerate=1    ; 1 = Uncapped framerate (-1 in Unity), 0 = Game default
Logging=1           ; 1 = Write BeneathDeltaTimeFix.log for diagnostics, 0 = Disabled

Note: You can edit TargetHz while playing; the mod detects changes automatically
within 1 second and applies the new timestep and gravity scale without restarting.

UNINSTALLATION:
Delete "dxgi.dll", "BeneathDeltaTimeFix.ini", and "BeneathDeltaTimeFix.log" from
the game directory.
