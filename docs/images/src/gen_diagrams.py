"""Generates system-architecture, pointing-geometry and pass-timeline SVGs. Usage: python3 gen_diagrams.py ../"""
import math, sys

OUTDIR = sys.argv[1]
FONT = "Helvetica, Arial, sans-serif"


class Svg:
    def __init__(self, w, h):
        self.w, self.h, self.el = w, h, []
        self.add(f'<defs><marker id="arr" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="8" markerHeight="8" orient="auto-start-reverse">'
                 f'<path d="M0,0 L10,5 L0,10 z" fill="#374151"/></marker></defs>')
        self.add(f'<rect x="0" y="0" width="{w}" height="{h}" fill="#ffffff"/>')

    def add(self, s): self.el.append(s)

    def text(self, x, y, s, size=13, anchor="start", fill="#111827", weight="normal", extra=""):
        self.add(f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" fill="{fill}" '
                 f'font-weight="{weight}" font-family="{FONT}" {extra}>{s}</text>')

    def rect(self, x, y, w, h, fill, stroke, rx=10, dashed=False, sw=2):
        d = 'stroke-dasharray="7 5"' if dashed else ""
        self.add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}" {d}/>')

    def box(self, x, y, w, h, fill, stroke, title, lines=(), dashed=False):
        self.rect(x, y, w, h, fill, stroke, dashed=dashed)
        self.text(x + w / 2, y + 22, title, 14, "middle", stroke, "bold")
        for i, l in enumerate(lines):
            self.text(x + w / 2, y + 41 + 16 * i, l, 11.5, "middle", "#374151")

    def line(self, x1, y1, x2, y2, color="#374151", w=2, arrow=False, both=False, dashed=False):
        m = ' marker-end="url(#arr)"' if arrow else ""
        if both: m += ' marker-start="url(#arr)"'
        d = ' stroke-dasharray="6 4"' if dashed else ""
        self.add(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="{w}"{m}{d}/>')

    def path(self, d, color="#374151", w=2, fill="none", arrow=False, dashed=False):
        m = ' marker-end="url(#arr)"' if arrow else ""
        dd = ' stroke-dasharray="6 4"' if dashed else ""
        self.add(f'<path d="{d}" stroke="{color}" stroke-width="{w}" fill="{fill}"{m}{dd}/>')

    def label(self, x, y, s, color="#374151", size=11):
        self.text(x, y, s, size, "middle", color, "bold")

    def save(self, name):
        open(f"{OUTDIR}/{name}", "w").write(
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.w}" height="{self.h}" viewBox="0 0 {self.w} {self.h}">'
            + "".join(self.el) + "</svg>")
        print("wrote", name)


BLUE, AMBER, GREEN, PINK, RED, GRAY, PURPLE = "#1d4ed8", "#b45309", "#15803d", "#be185d", "#b91c1c", "#6b7280", "#6d28d9"

# =====================================================================
# 1. System architecture
# =====================================================================
s = Svg(1360, 700)
s.text(30, 38, "ArduSat Tracker — System Architecture", 22, "start", "#111827", "bold")
s.text(30, 60, "Where the data comes from, what the ESP32 does with it, and how it becomes antenna motion", 12, "start", "#4b5563")

# Internet zone
s.rect(20, 85, 300, 420, "#f8fafc", "#94a3b8", dashed=True, sw=1.5)
s.text(170, 108, "INTERNET", 12, "middle", "#64748b", "bold")
s.box(45, 125, 250, 120, "#ede9fe", PURPLE, "N2YO REST API",
      ["api.n2yo.com : 443 (HTTPS)", "/positions → az, el now", "/radiopasses → AOS / LOS", "needs free API key"])
s.box(45, 270, 250, 80, "#ede9fe", PURPLE, "NTP servers", ["pool.ntp.org", "time.nist.gov → UTC"])
s.box(45, 375, 250, 100, "#f1f5f9", "#475569", "Wi-Fi router", ["2.4 GHz WPA2", "(ESP32 has no 5 GHz)", "internet access required"])

# ESP32 zone
s.rect(380, 85, 560, 520, "#e0ecff", BLUE, rx=14, sw=2.5)
s.text(660, 112, "ESP32  ·  tracker-v2.ino", 16, "middle", BLUE, "bold")
mods = [
    (400, 130, "Network layer", ["WiFiClientSecure + HttpClient", "auto-reconnect every 10 s"]),
    (670, 130, "Time", ["configTime(UTC) / time()", "UTC epoch seconds"]),
    (400, 225, "API client + JSON", ["obtenerPosicionActual()", "actualizarPase() · ArduinoJson"]),
    (670, 225, "Pass scheduler", ["IDLE · PREPASS · INPASS", "PARKING · HOMING"]),
    (400, 320, "Motion control", ["deadband · rate limit", "backlash · az wrap / limits"]),
    (670, 320, "AccelStepper ×2", ["accel/decel profiles", "TRACK · PARK · HOME"]),
    (400, 415, "UI / Menu", ["TFT status screen", "encoder: 25 settings + 5 actions"]),
    (670, 415, "Config store", ["NVS Preferences 'tracker'", "magic + version + CRC32"]),
]
for x, y, t, l in mods:
    s.box(x, y, 250, 78, "#ffffff", BLUE, t, l)
s.text(660, 530, "Single loop(): run steppers → Wi-Fi check → encoder/menu →", 11.5, "middle", "#1e3a8a")
s.text(660, 547, "state update → API calls when their interval expires", 11.5, "middle", "#1e3a8a")
s.text(660, 575, "Intervals adapt to state (1 s in pass … 30 s idle)", 11.5, "middle", "#1e3a8a", "bold")

# internal arrows
s.line(525, 208, 525, 225, BLUE, arrow=True)
s.line(795, 208, 795, 225, BLUE, arrow=True)
s.line(650, 264, 670, 264, BLUE, arrow=True)
s.line(795, 303, 795, 320, BLUE, arrow=True)
s.line(650, 359, 670, 359, BLUE, arrow=True)
s.line(670, 454, 650, 454, BLUE, both=True)

# Internet ↔ ESP arrows
s.line(295, 185, 400, 185, PURPLE, both=True); s.label(347, 178, "HTTPS", PURPLE)
s.path("M295,300 L350,300 L350,74 L860,74 L860,130", PURPLE, arrow=True, dashed=True); s.label(600, 69, "NTP (UDP 123)", PURPLE)
s.line(295, 425, 380, 425, "#475569", both=True); s.label(337, 418, "Wi-Fi", "#475569")

# Right: power + mechanics
s.box(1000, 110, 330, 95, "#fce7f3", PINK, "2 × Stepper drivers", ["STEP / DIR / EN (3.3 V logic)", "TMC2209 · A4988 · DRV8825 · DM542"])
s.box(1000, 230, 330, 80, "#fce7f3", PINK, "2 × Stepper motors", ["NEMA17 / NEMA23, 200 steps/rev"])
s.box(1000, 335, 330, 95, "#fef3c7", AMBER, "Az/El rotator mechanics", ["gear / belt / worm reduction", "bearings · cable wrap · end stops"])
s.box(1000, 455, 330, 80, "#dcfce7", GREEN, "Antenna", ["Yagi / LPDA for 2 m + 70 cm"])
s.box(1000, 560, 330, 95, "#f1f5f9", "#475569", "Transceiver / SDR", ["not controlled by firmware", "Doppler tuned manually / by PC"])
s.path("M920,359 L965,359 L965,158 L1000,158", PINK, arrow=True)
s.text(982, 262, "STEP / DIR / EN ×2", 11, "middle", PINK, "bold", 'transform="rotate(-90 982 262)"')
for y1, y2 in ((205, 230), (310, 335), (430, 455), (535, 560)):
    s.line(1165, y1, 1165, y2, "#374151", arrow=True)
s.text(1175, 550, "coax", 11, "start", "#374151")

# HMI + power
s.box(400, 625, 250, 60, "#fef3c7", AMBER, "TFT ILI9341 + KY-040", ["status + menu"])
s.line(525, 605, 525, 625, AMBER, both=True)
s.box(690, 625, 250, 60, "#fee2e2", RED, "12–24 V PSU + 5 V buck", ["motors + ESP32"])
s.line(815, 605, 815, 625, RED, arrow=False)
s.save("system-architecture.svg")

# =====================================================================
# 2. Geometry + drivetrain
# =====================================================================
g = Svg(1360, 620)
g.text(30, 38, "Pointing Geometry & Drivetrain", 22, "start", "#111827", "bold")
g.text(30, 60, "N2YO returns azimuth/elevation for YOUR latitude/longitude/altitude — the rotator just has to reproduce those two angles",
       12, "start", "#4b5563")

# Panel A: top view (azimuth)
cx, cy, r = 230, 330, 170
g.text(cx, 100, "A · Top view — azimuth", 15, "middle", BLUE, "bold")
g.add(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="#f8fafc" stroke="#94a3b8" stroke-width="2"/>')
for e in (30, 60):
    rr = r * (90 - e) / 90
    g.add(f'<circle cx="{cx}" cy="{cy}" r="{rr}" fill="none" stroke="#cbd5e1" stroke-dasharray="4 4"/>')
    g.text(cx + 4, cy - rr + 13, f"{e}°", 10, "start", "#94a3b8")
g.text(cx + 4, cy - r + 13, "0° (horizon)", 10, "start", "#94a3b8")
for ang, lab in ((0, "N 0°"), (90, "E 90°"), (180, "S 180°"), (270, "W 270°")):
    a = math.radians(ang)
    x, y = cx + (r + 22) * math.sin(a), cy - (r + 22) * math.cos(a) + 4
    g.text(x, y, lab, 12, "middle", "#111827", "bold")
    g.line(cx + r * math.sin(a), cy - r * math.cos(a), cx + (r - 10) * math.sin(a), cy - (r - 10) * math.cos(a), "#64748b")
# sample pass: AOS at az 300 el 0 → TCA az 30 el 55 → LOS az 120 el 0
pts = []
for i in range(41):
    t = i / 40
    az = 300 + 180 * t
    el = 55 * math.sin(math.pi * t)
    rr = r * (90 - el) / 90
    pts.append((cx + rr * math.sin(math.radians(az)), cy - rr * math.cos(math.radians(az))))
g.path("M" + " L".join(f"{x:.1f},{y:.1f}" for x, y in pts), GREEN, 2.5, arrow=True)
for (x, y), lab in ((pts[0], "AOS"), (pts[20], "TCA"), (pts[-1], "LOS")):
    g.add(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="5" fill="{GREEN}"/>')
    g.text(x + (8 if lab != "AOS" else -8), y - 8, lab, 11, "start" if lab != "AOS" else "end", GREEN, "bold")
# azimuth arc to TCA
tx, ty = pts[20]
g.line(cx, cy, tx, ty, BLUE, 2, dashed=True)
g.path(f"M{cx},{cy-38} A38,38 0 0,1 {cx+38*math.sin(math.radians(30)):.1f},{cy-38*math.cos(math.radians(30)):.1f}", BLUE, 2, arrow=True)
g.line(cx, cy, cx, cy - r, BLUE, 1, dashed=True)
g.text(cx - 6, cy - 44, "az", 12, "end", BLUE, "bold")
g.add(f'<circle cx="{cx}" cy="{cy}" r="5" fill="#111827"/>')
g.text(cx, cy + 22, "you", 11, "middle", "#111827")
g.text(cx, 545, "Azimuth: clockwise from TRUE north (not magnetic)", 11.5, "middle", "#374151")
g.text(cx, 562, "Centre = zenith (90°), edge = horizon (0°)", 11.5, "middle", "#374151")

# Panel B: side view (elevation)
ox, oy = 520, 470
g.text(640, 100, "B · Side view — elevation", 15, "middle", AMBER, "bold")
g.line(440, oy, 840, oy, "#64748b", 2)
g.text(830, oy + 18, "horizon (0°)", 11, "end", "#64748b")
g.add(f'<rect x="{ox-12}" y="{oy-110}" width="24" height="110" fill="#e5e7eb" stroke="#4b5563" stroke-width="2"/>')
g.text(ox - 18, oy - 50, "mast", 11, "end", "#4b5563")
px, py = ox, oy - 110
g.add(f'<circle cx="{px}" cy="{py}" r="9" fill="#fde68a" stroke="{AMBER}" stroke-width="2"/>')
el = math.radians(35)
bx, by = px + 200 * math.cos(el), py - 200 * math.sin(el)
g.line(px - 60 * math.cos(el), py + 60 * math.sin(el), bx, by, "#111827", 5)
for k in range(1, 6):
    ex_, ey_ = px + (k * 35 - 40) * math.cos(el), py - (k * 35 - 40) * math.sin(el)
    dx, dy = 22 * math.sin(el), 22 * math.cos(el)
    g.line(ex_ - dx, ey_ - dy, ex_ + dx, ey_ + dy, "#111827", 3)
g.line(bx, by, bx + 90 * math.cos(el), by - 90 * math.sin(el), GREEN, 2, arrow=True, dashed=True)
g.text(bx + 95 * math.cos(el), by - 95 * math.sin(el) - 6, "to satellite", 12, "start", GREEN, "bold")
g.line(px, py, px + 230, py, "#94a3b8", 1.5, dashed=True)
g.path(f"M{px+110},{py} A110,110 0 0,0 {px+110*math.cos(el):.1f},{py-110*math.sin(el):.1f}", AMBER, 2, arrow=True)
g.text(px + 120, py - 30, "el", 13, "start", AMBER, "bold")
g.text(640, 520, "Elevation: 0° at horizon → 90° at zenith", 11.5, "middle", "#374151")
g.text(640, 537, "Firmware clamps to EL Min…EL Max (−5…90° default)", 11.5, "middle", "#374151")
g.text(640, 554, "Below 0° during PREPASS = pre-positioned at AOS azimuth", 11.5, "middle", "#374151")

# Panel C: drivetrain
g.text(1085, 100, "C · Drivetrain & steps/degree", 15, "middle", PINK, "bold")
chain = [("Stepper motor", "200 full steps/rev (1.8°)"), ("Driver microstepping", "×16 (MS pins / TMC config)"),
         ("Mechanical reduction", "e.g. 5:1 belt or 50:1 worm"), ("Antenna axis", "Az 0–360° · El 0–90°")]
for i, (t, sub) in enumerate(chain):
    y = 125 + i * 82
    g.box(960, y, 250, 58, "#fce7f3" if i < 2 else "#fef3c7", PINK if i < 2 else AMBER, t, [sub])
    if i < 3:
        g.line(1085, y + 58, 1085, y + 82, "#374151", arrow=True)
g.rect(925, 465, 320, 125, "#f9fafb", "#d1d5db", sw=1.5)
g.text(1085, 487, "steps/deg = steps_rev × microsteps × ratio / 360", 12, "middle", "#111827", "bold")
g.text(1085, 510, "200 × 16 × 5 / 360 = 44.4  (belt 5:1)", 12, "middle", "#374151")
g.text(1085, 530, "200 × 16 × 50 / 360 = 444.4  (worm 50:1)", 12, "middle", "#374151")
g.text(1085, 554, "Firmware default = 10.0 → set it in the menu", 11.5, "middle", RED, "bold")
g.text(1085, 574, "max speed (°/s) = TR MaxSpd ÷ steps/deg", 11.5, "middle", "#374151")
g.save("pointing-geometry.svg")

# =====================================================================
# 3. Pass timeline
# =====================================================================
t = Svg(1360, 520)
t.text(30, 38, "One Satellite Pass — What the Firmware Does", 22, "start", "#111827", "bold")
t.text(30, 60, "tracker-v2.ino state machine vs. time (default config: prepass = 600 s, parkWhenIdle = ON)", 12, "start", "#4b5563")
X0, X1 = 200, 1320
segs = [("PARKED / IDLE", 200, 380, "#f3f4f6", GRAY), ("PREPASS", 380, 600, "#fef9c3", AMBER),
        ("INPASS", 600, 1000, "#dcfce7", GREEN), ("PARKING", 1000, 1150, "#e0f2fe", "#0369a1"),
        ("PARKED / IDLE", 1150, 1320, "#f3f4f6", GRAY)]
# elevation curve
t.text(30, 130, "Satellite", 13, "start", "#111827", "bold")
t.text(30, 147, "elevation", 13, "start", "#111827", "bold")
t.line(X0, 190, X1, 190, "#94a3b8", 1.5)
t.text(X0 - 8, 194, "0°", 11, "end", "#64748b")
pts = []
for i in range(81):
    x = 380 + (1120 - 380) * i / 80
    u = (x - 600) / 400
    if 0 <= u <= 1:
        el = 60 * math.sin(math.pi * u)
    else:
        d = -u if u < 0 else u - 1
        el = -14 * (1 - math.exp(-8 * d))
    pts.append((x, 190 - el * 1.25))
t.path("M" + " L".join(f"{x:.1f},{y:.1f}" for x, y in pts), GREEN, 3)
t.text(800, 105, "TCA (max elevation)", 11, "middle", GREEN, "bold")
for x, lab in ((600, "AOS · startUTC"), (1000, "LOS · endUTC")):
    t.line(x, 85, x, 470, "#111827", 1.5, dashed=True)
    t.text(x, 80, lab, 12, "middle", "#111827", "bold")
t.line(380, 85, 380, 470, AMBER, 1.5, dashed=True)
t.text(380, 80, "AOS − prepassSec", 12, "middle", AMBER, "bold")

rows = [("State", 230), ("Motors", 290), ("/positions every", 350), ("/radiopasses every", 410)]
for name, y in rows:
    t.text(30, y + 22, name, 13, "start", "#111827", "bold")
cells = {
    "Motors": ["disabled", "enabled · track", "enabled · track", "enabled · park", "disabled"],
    "/positions every": ["30 s", "5 s", "1 s *", "30 s", "30 s"],
    "/radiopasses every": ["300 s", "60 s", "60 s", "300 s", "300 s"],
}
for (name, x0, x1, fill, stroke) in segs:
    t.rect(x0 + 2, 230, x1 - x0 - 4, 36, fill, stroke, rx=6)
    t.text((x0 + x1) / 2, 253, name, 13, "middle", stroke, "bold")
for r_i, (name, y) in enumerate(rows[1:]):
    for c_i, (_, x0, x1, fill, stroke) in enumerate(segs):
        t.rect(x0 + 2, y, x1 - x0 - 4, 36, "#ffffff", "#d1d5db", rx=6, sw=1)
        t.text((x0 + x1) / 2, y + 23, cells[name][c_i], 12, "middle", "#374151")
t.text(30, 488, "* A 10-minute pass at 1 s ≈ 600 /positions calls (limit 1000/h). Each HTTPS call blocks loop() for ~0.3–2 s, so real cadence is slower and motion pauses briefly.",
       11.5, "start", "#374151")
t.text(30, 506, "Antenna moves only when the target leaves the deadband (beamwidth × factor = 20° × 0.5 = 10° by default), at most every 800 ms per axis.",
       11.5, "start", "#374151")
t.save("pass-timeline.svg")
