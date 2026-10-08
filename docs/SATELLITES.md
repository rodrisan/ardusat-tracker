# Satellite Catalogue — What You Can Track

> Companion to the [Build & Operation Guide](SATELLITE_TRACKER_GUIDE.md).
> **Status checked: 8 October 2026.** Satellite health changes week to week, so **always confirm on the
> [AMSAT live status page](https://www.amsat.org/status/)** (and [ARISS](https://www.ariss.org/current-status-of-iss-stations) for the ISS) before planning a session.

The tracker follows **one satellite at a time**, chosen by `noradID` in `tracker-v2.ino`. Any object N2YO knows can be tracked. This page lists the ones worth pointing a ham antenna at, what radio you need for each, and how each one affects the firmware's settings and API budget.

---

## 1. Quick picks

| If you have… | Start with | Why |
|---|---|---|
| Dual-band FM handheld + handheld Yagi | **SO-50**, **AO-123**, **ISS** (voice repeater when enabled; APRS intermittent) | FM, no SSB needed, 10–15 min passes |
| Receive-only SDR | **Meteor-M N2-3 / N2-4** (weather images), **ISS** SSTV events / repeater downlink when ARISS enables them | No licence needed to receive in most countries |
| All-mode full-duplex radio (SSB/CW) | **RS-44**, **FO-29**, **AO-7** | Linear transponders with many simultaneous QSOs. RS-44 gives long passes and wide coverage. |
| 2.4 GHz uplink + 10 GHz dish | **QO-100** (only inside its footprint, §5) | Geostationary, so no tracking needed |

---

## 2. FM repeater satellites ("FM birds")

The easiest way to start: an FM dual-band radio, a small V/U Yagi, and manual Doppler on the 70 cm side.

| Name | NORAD | Uplink | Downlink | Tone | Status (Oct 2026) | Notes |
|---|---|---|---|---|---|---|
| **ISS** voice repeater | 25544 | 145.990 MHz FM | 437.800 MHz FM | 67.0 Hz | ✅ Active when ARISS enables it | Shares the radio with APRS and school contacts. ARISS announces downtime. |
| **ISS** APRS digipeater | 25544 | 145.825 MHz | 145.825 MHz | — | ⚠️ Intermittent (check ARISS) | 1200 bd AFSK. ARISS reports in 2026 show the radios mostly in voice/repeater configuration. |
| **ISS** voice (school contacts, SSTV) | 25544 | 144.490 (ITU R2/R3) · 145.200 (R1) | 145.800 MHz FM | — | ⏱ Scheduled | Listen-only for most stations. SSTV events are announced by ARISS. |
| **SO-50** (SaudiSat-1C) | 27607 | 145.850 MHz FM | 436.795 MHz FM | 67.0 Hz | ✅ Active | **Arm the 10-min timer** first: 2 s carrier with **74.4 Hz** tone. |
| **AO-123** (ASRTU-1) | 61781 | 145.850 MHz FM | 435.400 MHz FM | 67.0 Hz | ✅ Reported active | Launched Nov 2024. ~460 km orbit, so passes are short (≈ 10 min). |
| **AO-91** (Fox-1B) | 43017 | 435.250 MHz FM | 145.960 MHz FM | none (carrier-operated; older lists say 67 Hz) | ⚠️ End of life | Sunlight-only (AMSAT, Apr 2025). Don't try it in eclipse. Reports are sparse (last SatNOGS observation seen: Apr 2026). |
| **PO-101** (Diwata-2) | 43678 | 437.500 MHz FM | 145.900 MHz FM | 141.3 Hz | ❌ Likely non-operational | Was scheduled-only with a 10-min timer. Listed as "long non-operational" in Jan 2026 frequency sheets. |
| **SO-124** (HADES-R) | 62690 | 145.925 MHz | 436.885–436.888 MHz | — | ❌ Re-entered 3 Feb 2026 | Listed so you don't waste a pass on it. |
| **SO-125** (HADES-ICM) | 63492 | 145.875 MHz | 436.666 MHz | none | ❌ Re-entered ~May 2026 | SatNOGS marked its transmitters inactive on 23 May 2026 ("satellite decayed"). |

**AO-91 and PO-101 are "upside-down":** they have a 70 cm uplink and a 2 m downlink, so you apply Doppler correction on the *uplink*.

---

## 3. Linear-transponder satellites (SSB / CW)

These need an all-mode radio (or two) that can transmit and receive at the same time, plus continuous Doppler correction. A tracker that keeps the beam on the satellite matters most here.

| Name | NORAD | Uplink (LSB/CW) | Downlink (USB/CW) | Inverting | Beacon | Status (Oct 2026) | Pass length |
|---|---|---|---|---|---|---|---|
| **RS-44** (DOSAAF-85) | 44909 | 145.935–145.995 MHz | 435.610–435.670 MHz | yes | 435.605 MHz CW | ✅ Active (demonstrated live Apr 2026) | **~17–20 min** (1175 × 1511 km) |
| **FO-29** (JAS-2) | 24278 | 145.900–146.000 MHz | 435.800–435.900 MHz | yes | 435.795 MHz CW | ✅ Sunlight only (full-sun season to ~mid-Nov 2026) | ~15–20 min |
| **AO-7** Mode A | 7530 | 145.850–145.950 MHz | 29.400–29.500 MHz | no | 29.502 MHz | ✅ Sunlight only | ~20 min (~1450 km) |
| **AO-7** Mode B | 7530 | 432.125–432.175 MHz | 145.925–145.975 MHz | yes | 145.9775 MHz CW | ✅ Sunlight only | ~20 min |

* **Inverting** means that tuning your uplink *up* moves the downlink *down*. Use LSB up and USB down.
* AO-7 Mode A needs a **10 m (29 MHz)** receive antenna, which a V/U Yagi on the rotator doesn't cover. A simple fixed dipole works.
* "Sunlight only" means the batteries are dead, so the satellite works only while its panels are lit. Expect it to cut out when it enters Earth's shadow mid-pass.

---

## 4. Weather satellites (receive only, 137 MHz)

| Name | NORAD | Downlink | Mode | Status (Oct 2026) | Notes |
|---|---|---|---|---|---|
| **Meteor-M N2-3** | 57166 | 137.900 MHz | LRPT 72 or 80 kbps (QPSK) | ✅ Operational with limits | Antenna not fully deployed, so the signal is weaker. A tracked Yagi helps. |
| **Meteor-M N2-4** | 59051 | 137.900 MHz (137.100 backup) | LRPT 72 or 80 kbps | ✅ Operational | Sun-synchronous orbit at ~820 km, ~15 min passes |
| ~~NOAA-15 / 18 / 19~~ | 25338 / 28654 / 33591 | ~~137.62 / 137.9125 / 137.10~~ | APT | ❌ **Decommissioned** Jun–Aug 2025 | Old tutorials still list them. Don't plan around them. |

* Decode LRPT with **SatDump**. You need a 137 MHz antenna. A QFH or V-dipole works without a rotator. A tracked 137 MHz Yagi improves weak N2-3 passes. The signal is circularly polarised, so a linear Yagi loses about 3 dB but still works.
* The V/U Yagi on the rotator is **not** resonant at 137 MHz. Use a dedicated antenna.

---

## 5. Geostationary: QO-100 (Es'hail-2)

| Name | NORAD | Uplink | Downlink | Position |
|---|---|---|---|---|
| **QO-100** narrowband | 43700 | 2400.050–2400.300 MHz (RHCP) | 10489.550–10489.800 MHz (linear V) | 25.9° E, fixed |

* **Footprint:** roughly Brazil to Thailand (Europe, Africa, the Middle East, most of Asia). It is **not visible from North America or western South America**. From the default location in the code (Guadalajara, MX), N2YO will always report it below the horizon.
* There is nothing to track, so you point a dish once. The firmware only moves the antenna in `PREPASS`/`INPASS`, and a GEO satellite never produces a "pass", so the tracker would sit at park. If you are inside the footprint, set `parkAzDeg`/`parkElDeg` (in `TrackerConfig`) to QO-100's az/el from n2yo.com and the rotator becomes a dish aimer.

---

## 6. Retired or dead (don't bother)

| Name | NORAD | Why |
|---|---|---|
| IO-117 (GreenCube) | 53106 | MEO digipeater (435.310 MHz). Stopped responding in 2024. |
| PO-101 (Diwata-2) | 43678 | Long non-operational (see §2) |
| SO-124 (HADES-R) | 62690 | Re-entered Feb 2026 |
| SO-125 (HADES-ICM) | 63492 | Re-entered ~May 2026 |
| NOAA-15 / 18 / 19 | 25338 / 28654 / 33591 | Decommissioned 2025 |

---

## 7. How each satellite type affects the firmware

The firmware's defaults (`tracker-v2.ino`) were written with ~10-minute LEO passes in mind. Higher orbits mean **longer passes**, which affects the API budget much more than it affects motion.

| Orbit class | Examples | Typical pass | `/positions` calls in one pass at the default 1 s | Fits 1000/h? | Max angular rate |
|---|---|---|---|---|---|
| Low LEO (400–500 km) | ISS, AO-123 | 5–11 min | 300–660 | ✅ | Highest. Fast az slews near overhead. |
| LEO (600–850 km) | SO-50, Meteor-M | 10–15 min | 600–900 + prepass/idle | ⚠️ tight | High |
| High LEO (1200–1500 km) | RS-44, AO-7, FO-29 | **15–22 min** | **900–1320 + 120 prepass** | ❌ **exceeds** | Moderate |
| MEO (~5800 km) | IO-117 (dead), future MEO birds | 1 h + | 3600+ | ❌❌ | Very slow |
| GEO | QO-100 | — (always up or never) | — | n/a (no pass, see §5) | 0 |

### Suggested change (not applied): poll less often during a pass

With the default 10° deadband (`BW 20 × 0.5`) and LEO angular rates of roughly 0.1–0.5°/s for most of a pass (≈ 1°/s only near zenith on high passes), asking for the position every second gains almost nothing. A **3 s** interval keeps tracking error well inside the deadband and cuts usage by about 3×:

```cpp
// tracker-v2.ino → updateIntervalsFromState()
if (trackState == INPASS) {
  posIntervalMs  = 3000;   // was 1000 — 20-min RS-44 pass ≈ 400 calls instead of 1200
  passIntervalMs = 60000;
}
```

| Satellite class | Suggested `INPASS` position interval |
|---|---|
| ISS / AO-123 / SO-50 | 2–3 s |
| RS-44 / FO-29 / AO-7 | 3–5 s |
| MEO | 10–20 s |

For long passes, the more robust fixes are in the guide's [§13.1](SATELLITE_TRACKER_GUIDE.md#131-network-calls-block-motion-highest-impact): one `/positions/.../300/` call every ~4 minutes, or local SGP4 propagation.

### Other per-type settings

| Setting | FM LEO (Yagi) | Linear (narrower Yagi) | Weather (137 MHz Yagi) |
|---|---|---|---|
| `BW Az / El` | 40–60° | 25–40° | 60–70° |
| `Deadband` | 0.3 | 0.25 | 0.3 |
| `Prepass` | 600 s | 600 s | 300 s |
| Min. pass elevation (in `actualizarPase()` change `"/1/0/&apiKey="` to e.g. `"/1/15/&apiKey="`) | 15–20° | 10° | 20° |

---

## 8. Switching satellites

The NORAD ID is compiled in, so:

1. Change `const int noradID = …;` in `tracker-v2.ino`.
2. Re-flash. Saved menu settings (NVS) are kept.
3. Adjust `BW` / `Deadband` in the menu for the antenna you're using, and **SAVE**.

**Suggested improvement (not applied, firmware unchanged):** a small list plus a menu entry so you can pick the satellite on the device. One way to do it:

```cpp
// Place above obtenerPosicionActual() and menuItems[] (e.g. right after the WIFI / API block)
struct SatEntry { const char* name; uint32_t norad; };
const SatEntry kSats[] = {
  {"ISS", 25544}, {"SO-50", 27607}, {"AO-123", 61781},
  {"RS-44", 44909}, {"FO-29", 24278}, {"AO-7", 7530},
  {"METEOR N2-3", 57166}, {"METEOR N2-4", 59051},
};
const uint32_t kSatCount = sizeof(kSats) / sizeof(kSats[0]);

// TrackerConfig: add  uint32_t satIndex = 0;
//   Must be uint32_t: menuHandle() writes IT_UINT32 items through a uint32_t*.
//   Change version 1 -> 2 in THREE places: the struct default, validateCfg()
//   and saveConfig() (inout.version = 1;). Missing saveConfig() makes every
//   SAVE fail validation, so settings reset at each boot.
//   validateCfg(): if (c.satIndex >= kSatCount) return false;
//
// Both URL builders: replace String(noradID) with String(kSats[cfg.satIndex].norad)
//
// menuItems[]:
//   {"Satellite", IT_UINT32, &cfg.satIndex, 0,0,0, 0,0,0, 1, 0, kSatCount - 1, "", ACT_EXIT},
```

Compile-checked (on top of the compile fix in branch `claude/tracker-v2-compile-fix`, esp32 core 3.0.4). Remember to clear `tiempoInicioPase/tiempoFinPase` and force an `actualizarPase()` after switching.

---

## 9. Finding more satellites

* **N2YO amateur-radio category:** [n2yo.com/satellites/?c=18](https://www.n2yo.com/satellites/?c=18). The API's `/above/{lat}/{lng}/{alt}/{radius}/18/` lists amateur satellites currently above you. It counts against the **100/hour** limit, so don't call it in a loop.
* **AMSAT:** [live status](https://www.amsat.org/status/), [FM frequency list](https://www.amsat.org/live-fm-satellites/), [linear frequency list](https://www.amsat.org/live-linear-satellites/).
* **SatNOGS DB:** [db.satnogs.org](https://db.satnogs.org/) has frequencies, modes, operational/decayed status and recent observations.
* **hams.at:** see who is planning to work which pass, e.g. [AO-123](https://hams.at/sats/61781).
* **ARISS:** [ISS radio status](https://www.ariss.org/current-status-of-iss-stations).

---

### Sources (checked 8 Oct 2026)

* AMSAT: [status](https://www.amsat.org/status/), [FM satellites](https://www.amsat.org/live-fm-satellites/), [linear satellites](https://www.amsat.org/live-linear-satellites/), [SO-50](https://www.amsat.org/two-way-satellites/so-50-satellite-information/), [AO-91](https://www.amsat.org/two-way-satellites/ao-91/), [RS-44](https://www.amsat.org/two-way-satellites/rs-44/)
* [ARISS: current status of ISS stations](https://www.ariss.org/current-status-of-iss-stations)
* [AMSAT-SE: AO-123 continuous service](https://www.amsat.se/2025/08/04/ao-123-fm-transponder-to-enter-continuous-service/), [FO-29 update April 2026](https://www.amsat.se/2026/04/12/fo-29-update-april-2026/), [SO-125 designation](https://www.amsat.se/2025/06/01/amsat-eas-hades-icm-now-so-125/)
* SatNOGS DB: [HADES-R](https://db.satnogs.org/satellite/EIPI-1482-6978-9759-4787/), [HADES-ICM](https://db.satnogs.org/satellite/TPAP-4475-7915-0965-4305/), [Meteor M2-3](https://db.satnogs.org/satellite/TLCE-8079-2988-7210-7028/), [Meteor M2-4](https://db.satnogs.org/satellite/59051), [QO-100](https://db.satnogs.org/satellite/43700)
* [WMO OSCAR: Meteor-M N2-3](https://space.oscar.wmo.int/satellites/view/meteor_m_n2_3), [N2-4](https://space.oscar.wmo.int/satellites/view/meteor_m_n2_4)
* [AMSAT-UK: GreenCube IO-117 update](https://amsat-uk.org/2024/09/15/greencube-io-117-update/), [QO-100](https://amsat-uk.org/satellites/geo/eshail-2/)
* [NOAA OSPO: POES status](https://www.ospo.noaa.gov/Operations/POES/status.html)
