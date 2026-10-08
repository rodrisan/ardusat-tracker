# ArduSat Tracker

This tracker is aimed at ham radio operators who want to aim and track an antenna at overhead satellites.
It works using [N2YO's API](https://www.n2yo.com/api/): an ESP32 fetches the satellite's live azimuth/elevation
and the next pass window, then drives two stepper motors (azimuth + elevation) to point the antenna.

**PLEASE BE CAREFUL NOT TO OVERUSE THE API.** It is rate limited per endpoint and per hour
(1000 `/positions`, 100 `/radiopasses`). See [API budget](docs/SATELLITE_TRACKER_GUIDE.md#11-n2yo-api-budget).

Currently using STEP/DIR commands for stepper motors. A direct-drive BLDC version is in progress.

## 📘 Documentation

**➡️ [Build & Operation Guide](docs/SATELLITE_TRACKER_GUIDE.md)** covers what you need, wiring, mechanics,
flashing, calibration, tracking a real pass, the menu reference, troubleshooting and known limitations.

| Schematic / diagram | |
|---|---|
| [Wiring schematic](docs/images/wiring-schematic.svg) | ESP32 ↔ TFT, encoder, drivers, motors, power |
| [System architecture](docs/images/system-architecture.svg) | Data flow from N2YO to antenna motion |
| [Pointing geometry & drivetrain](docs/images/pointing-geometry.svg) | Azimuth/elevation and the steps-per-degree formula |
| [Pass timeline](docs/images/pass-timeline.svg) | What the firmware does before, during and after a pass |
| [Bill of materials (CSV)](docs/BOM.csv) | Parts list |
| [Satellite catalogue](docs/SATELLITES.md) | Trackable satellites: NORAD IDs, frequencies, status, firmware settings per orbit type |

![Wiring schematic](docs/images/wiring-schematic.svg)

## Files

| File | Description |
|---|---|
| `tracker-v2.ino` | **Recommended.** AccelStepper motion, pass state machine, encoder menu, settings saved to flash |
| `ardusat-tracker.ino` | v1 / legacy. Blocking stepper loop, placeholder motor pins, TFT status only |

## Quick start

1. **Pick parts:** [Guide §3](docs/SATELLITE_TRACKER_GUIDE.md#3-bill-of-materials--what-you-need). Choose a driver and a microstep setting.
2. **Wire it:** follow the [schematic](docs/images/wiring-schematic.svg), set the microstep jumpers (§4.4), and run the **before first power-on** checks: buck at 5.00 V, driver orientation, Vref (§4.5).
3. **Install the toolchain** (§6.1) and copy `tracker-v2.ino` into **its own `tracker-v2` folder** (the repo holds two sketches, which won't compile together). ⚠️ Take `tracker-v2.ino` from branch `claude/tracker-v2-compile-fix`: the copy on `main` doesn't compile with current ESP32 tools ([details](docs/SATELLITE_TRACKER_GUIDE.md#64-build-and-upload)).
4. **Configure** Wi-Fi, API key, `noradID`, location and `stepsPerDegAz/El` in the sketch (§6.3, §5.3).
5. **Flash, then do the bench test** (§7.1).
6. **Align the antenna to true north and the horizon** and power up ≥ 20 min before AOS (§7.2, §8.2).

⚠️ Close the on-device menu with a **long press on a setting item**, never with its `EXIT` item ([why](docs/SATELLITE_TRACKER_GUIDE.md#138-menu-the-opening-click-also-activates-the-selected-item)).

Full details are in the [guide](docs/SATELLITE_TRACKER_GUIDE.md).
