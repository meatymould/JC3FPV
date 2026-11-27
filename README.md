# JC3FPV — standalone Acro proof of concept

Download [JC3FPV v0.1.0](downloads/JC3FPV-v0.1.0.zip?raw=true), extract it, start Just Cause 3 in single-player, and run `bin/JC3FPV.exe` from the extracted folder. It loads `JC3FPV.dll` directly from this folder. No installation into the game folder is required. If Steam was closed, start JC3 manually first and run the launcher again. Restarting JC3 removes the mod; it must be loaded each session.

| Control | Action |
|---|---|
| F6 | Open / close drone settings |
| F7 | Enable / disable FPV |
| F8 | Reset position, attitude and velocity to launch point |
| F9 | Reload `bin/JC3FPV.ini` |
| Left stick horizontal | Yaw |
| Left stick vertical | Throttle, bottom 0%, top 100% |
| Right stick horizontal | Roll |
| Right stick vertical | Pitch, push forward to pitch down |

Use an XInput-compatible controller (Xbox or a controller mapped through Steam Input). This is **Mode 2 Acro**. With a spring-centred gamepad, centre throttle is 50%, not altitude hold. For the default quad, approximate hover is 49.5% throttle. Lower the left stick before enabling, then add throttle immediately. `ThrottleMode=1` optionally puts throttle on the right trigger for easier gamepad practice. Native USB RC radio input is not implemented.

Keyboard fallback: W gives 65% throttle, Space adds 35%; arrow keys pitch/roll, A/D yaw. These are digital inputs and considerably less precise. Keyboard inputs can also reach Rico; gamepad input is isolated from the game through its XInput import.

The camera starts 2 metres above the current camera position with 25° upward tilt and 110° horizontal FOV. Settings include mass, thrust, drag, angular rates, expo, motor and rate response, throttle curve, camera tilt and FOV. Change the INI and press F9. Gravity is 9.80665 m/s². Physics uses 1 ms fixed steps, body-relative thrust and drag, quaternion attitude, motor lag and bounded angular acceleration. Releasing sticks stops rotation without self-levelling; momentum remains. There is no altitude hold.

## Drone settings (F6)

F6 opens a native settings window over the game. Flight simulation pauses while it is open. Use the sliders to preview weight, maximum thrust, optional speed limit, gravity (including zero gravity), pitch/roll and yaw speed, centre sensitivity, stick/throttle expo, air drag, motor/rotation response, camera tilt and FOV. **Save settings** keeps them for future sessions; **Restore defaults** returns the physics to the standard freestyle quad. F6, Escape or **Return to flight** closes the window and resumes flight. Unsaved previews still apply during the current session.

Weight is shown in grams, speed in km/h, rotation in degrees/sec and response in milliseconds. A speed limit of zero means unlimited. The default is normal Earth gravity and no speed cap. Higher thrust and less drag increase attainable speed without a hard cap. The optional speed cap limits total velocity, including dives, and is a custom gameplay setting. INI stores mass in kg, speed in m/s and response times in seconds. `Gravity` and `DragScale` are also configurable in the INI.

## Collision

Collision is enabled by default. A continuous five-ray approximation of a 0.18 m sphere queries JC3's native collision world after at most 5 ms of flight integration. It stops against loaded solid geometry, with a small clearance, low bounce and sliding friction. Physics remain fully Acro, including upside-down flight. The volume stays centred on the drone, independently of camera tilt.

INI settings: `Collision=1`, `CollisionRadius=0.18`, `CollisionSkin=0.015`, `CollisionBounce=0.08`, `CollisionFriction=0.18`. F9 reloads them. If the native collision backend becomes unavailable or faults, flight disables instead of silently continuing through the world. F8 recovers to the launch point.

## Prototype limitations

This is a camera-based flight prototype, **not a complete drone simulator**. There is no visible drone body, propeller damage or fully simulated contact torque. Collision uses a conservative ray fan rather than a Havok convex sphere sweep, so very thin geometry, edges and starting inside a solid can still need F8 recovery. Water is not treated as a solid floor. Rico remains at the launch area and world streaming at large distances is not yet integrated. Fly locally and use F8 if you go underground. Pausing on loss of foreground prevents background flight; in-game pause/menu state is not yet detected independently. The rate-controller response and aerodynamics are approximations, not measured replicas of Liftoff/TRYP or a particular real quad. No propwash, per-motor mixer, wind or ground effect.

Targets the independently inspected Steam executable with PE timestamp `0x5787641c` and image size `0x3241000`. Signature guards refuse unfamiliar or already patched camera functions. The launcher refuses detected JC3VR/CinematicTools modules; it does not remove or change them. JC3VR source was not used or changed.

## Build and validation

Run `build.cmd` with Visual Studio 2022/Build Tools, Desktop development with C++ and a Windows SDK installed. The script detects the compiler automatically. No external source dependencies. Eighteen automated checks include high-speed floor and wall impacts, ceiling contact, stable ground contact, unobstructed motion, landing frame-rate agreement and friction impulse bounds. Additional checks cover zero/custom gravity, the optional speed cap and zero-drag coasting. The original seven physics checks cover freefall, directional thrust, no levelling, 30/144 FPS agreement, unrestricted quaternion rotation, rate endpoints and drag. Runtime diagnostics are in `bin/JC3FPV.log`; launcher diagnostics in `bin/launcher.log`.

Developer test bridge: write `test` into `bin/command.txt` to perform a 12-second scripted climb/pitch/roll and restore normal camera. Other one-shot commands: `enable`, `disable`, `reset`, `settings` (toggle settings window), `probe` (six native rays), `collisiontest` (60 m/s drop and camera restore). These are internal validation commands, not an autopilot mode.

## Research and credits

Built with OpenAI Codex. Camera structure independently verified by disassembling the user's executable; public FOV-tool constants and the already installed CinematicTools offset list served as research leads, not copied implementation.

- [Betaflight setup guide](https://betaflight.com/docs/wiki/getting-started/setup-guide): Acro sticks command rotation rate.
- [Betaflight rate implementation](https://github.com/betaflight/betaflight/blob/master/src/main/fc/rc.c): Actual rate equation independently implemented.
- [Betaflight tuning notes](https://betaflight.com/docs/wiki/tuning/4-2-Tuning-Notes): rate response and thrust nonlinearity.
- [Liftoff simulation technology](https://www.liftoff-game.com/liftoff-simulations): physical forces, component response and aerodynamic behavior.
- [Bodycam official announcements](https://steamcommunity.com/app/2406770/announcements/): Acro drag and TRYP collaboration used as behavioral context.
- [JustC3 FOV Changer](https://github.com/Mrsuss60/JustC3_FOVChanger): camera manager/FOV layout research lead. No code copied.

No game files, saves, JC3VR code or third-party camera implementation are included in the distributable.
