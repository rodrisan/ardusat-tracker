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

![Wiring schematic](docs/images/wiring-schematic.svg)

## Files

| File | Description |
|---|---|
| `tracker-v2.ino` | **Recommended.** AccelStepper motion, pass state machine, encoder menu, settings saved to flash |
| `ardusat-tracker.ino` | v1 / legacy. Blocking stepper loop, placeholder motor pins, TFT status only |

## Quick start

1. Wire it as in the [schematic](docs/images/wiring-schematic.svg).
2. Install the ESP32 core plus the libraries ArduinoHttpClient, ArduinoJson, Adafruit GFX, Adafruit ILI9341 and AccelStepper.
3. In `tracker-v2.ino`, set `ssid`, `password`, `apiKey`, `noradID`, `latitude`, `longitude` and `altitude`.
4. Flash it, point the antenna at **true north and the horizon**, then power up.
5. In the menu, set `Steps/deg` for your gearing and choose **SAVE**.

Full details are in the [guide](docs/SATELLITE_TRACKER_GUIDE.md).
