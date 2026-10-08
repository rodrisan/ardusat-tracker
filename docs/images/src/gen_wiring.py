"""Generates wiring-schematic.svg (tracker-v2.ino pinout). Usage: python3 gen_wiring.py ../wiring-schematic.svg"""
import sys

OUT = sys.argv[1]
W, H = 1400, 1150
FONT = "Helvetica, Arial, sans-serif"
el = []

def add(s): el.append(s)

def text(x, y, s, size=13, anchor="start", fill="#111827", weight="normal", style=""):
    add(f'<text x="{x}" y="{y}" font-size="{size}" text-anchor="{anchor}" fill="{fill}" '
        f'font-weight="{weight}" font-family="{FONT}" {style}>{s}</text>')

def box(x, y, w, h, fill, stroke, title, sub=None, dashed=False):
    dash = 'stroke-dasharray="7 5"' if dashed else ""
    add(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="10" fill="{fill}" stroke="{stroke}" stroke-width="2" {dash}/>')
    text(x + w / 2, y + 22, title, 14, "middle", stroke, "bold")
    if sub:
        text(x + w / 2, y + 39, sub, 11, "middle", "#4b5563")

def wire(x1, y1, x2, y2, color="#1f2937", dashed=False, width=2):
    dash = 'stroke-dasharray="6 4"' if dashed else ""
    add(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="{width}" {dash}/>')

def dot(x, y, color="#1f2937"):
    add(f'<circle cx="{x}" cy="{y}" r="4" fill="{color}"/>')

def pin_left(x, y, label):   # pin on left edge of a box, label inside
    dot(x, y)
    text(x + 8, y + 4, label, 12)

def pin_right(x, y, label):  # pin on right edge of a box, label inside
    dot(x, y)
    text(x - 8, y + 4, label, 12, "end")

def netflag(x, y, name, color, anchor="right"):
    """Net label: small tag. anchor=side the wire comes from."""
    w = 8 * len(name) + 14
    if anchor == "right":   # wire enters from right, tag extends left
        x0 = x - w
    elif anchor == "left":
        x0 = x
    else:                   # 'bottom': tag above point
        x0 = x - w / 2
    if anchor == "top":
        x0 = x - w / 2
    yy = {"bottom": y - 24, "top": y + 2}.get(anchor, y - 11)
    add(f'<rect x="{x0}" y="{yy}" width="{w}" height="22" rx="4" fill="#ffffff" stroke="{color}" stroke-width="1.6"/>')
    text(x0 + w / 2, yy + 15, name, 11, "middle", color, "bold")

def gnd(x, y):
    wire(x, y, x, y + 10)
    wire(x - 10, y + 10, x + 10, y + 10)
    wire(x - 6, y + 15, x + 6, y + 15)
    wire(x - 2, y + 20, x + 2, y + 20)

def resistor_v(x, y_top, y_bot, label, color="#1f2937"):
    """Vertical resistor between y_top and y_bot (IEC box)."""
    mid = (y_top + y_bot) / 2
    wire(x, y_top, x, mid - 14, color)
    add(f'<rect x="{x-6}" y="{mid-14}" width="12" height="28" fill="#ffffff" stroke="{color}" stroke-width="2"/>')
    wire(x, mid + 14, x, y_bot, color)
    text(x + 10, mid + 4, label, 10, "start", "#374151")

def cap_v(x, y_top, y_bot, label):
    mid = (y_top + y_bot) / 2
    wire(x, y_top, x, mid - 4)
    wire(x - 10, mid - 4, x + 10, mid - 4)
    wire(x - 10, mid + 4, x + 10, mid + 4)
    wire(x, mid + 4, x, y_bot)
    text(x + 14, mid + 4, label, 10, "start", "#374151")

def cap_h(x_left, x_right, y, label):
    mid = (x_left + x_right) / 2
    wire(x_left, y, mid - 4, y)
    wire(mid - 4, y - 9, mid - 4, y + 9)
    wire(mid + 4, y - 9, mid + 4, y + 9)
    wire(mid + 4, y, x_right, y)
    text(mid, y - 13, label, 10, "middle", "#374151")

def sig(x1, x2, y, name, color="#1f2937"):
    wire(x1, y, x2, y, color)
    text((x1 + x2) / 2, y - 5, name, 10, "middle", color, "bold")

# ---------------------------------------------------------------- frame
add(f'<rect x="0" y="0" width="{W}" height="{H}" fill="#ffffff"/>')
text(30, 36, "ArduSat Tracker v2 — Wiring Schematic", 22, "start", "#111827", "bold")
text(30, 58, "Firmware: tracker-v2.ino  ·  ESP32 DevKit (WROOM-32)  ·  pin numbers are ESP32 GPIO numbers, not board silkscreen order",
     12, "start", "#4b5563")

C_PWR, C_ESP, C_TFT, C_ENC, C_DRV, C_HOME = "#b91c1c", "#1d4ed8", "#b45309", "#15803d", "#be185d", "#6b7280"

# ---------------------------------------------------------------- power section
OY = 80
box(40, OY + 10, 200, 70, "#fee2e2", C_PWR, "PSU 12–24 V DC", "≥ 3 A (2 × NEMA17)")
# + rail
wire(240, OY + 35, 300, OY + 35, C_PWR)
add(f'<rect x="300" y="{OY+27}" width="40" height="16" rx="3" fill="#ffffff" stroke="{C_PWR}" stroke-width="2"/>')
text(320, OY + 22, "F1 3–5 A", 10, "middle", C_PWR)
wire(340, OY + 35, 520, OY + 35, C_PWR)
dot(420, OY + 35, C_PWR)
wire(420, OY + 35, 420, OY + 5, C_PWR)
netflag(420, OY + 5, "+VMOT", C_PWR, "bottom")
# PSU GND
wire(240, OY + 62, 270, OY + 62)
gnd(270, OY + 62)

box(520, OY + 5, 200, 70, "#fee2e2", C_PWR, "Buck converter", "LM2596 / MP1584 → 5.0 V")
text(512, OY + 30, "IN+", 10, "end", C_PWR)
# buck outputs down to ESP32
wire(600, OY + 75, 600, 190, C_PWR)
text(606, OY + 98, "+5V", 11, "start", C_PWR, "bold")
wire(680, OY + 75, 680, 190)
text(686, OY + 98, "GND", 11, "start", "#1f2937", "bold")
dot(680, OY + 88)
wire(680, OY + 88, 760, OY + 88)
gnd(760, OY + 88)
wire(520, OY + 60, 495, OY + 60)
gnd(495, OY + 60)
text(512, OY + 55, "IN−", 10, "end", "#1f2937")

# ---------------------------------------------------------------- ESP32
EX, EY, EW, EH = 520, 190, 260, 700
add(f'<rect x="{EX}" y="{EY}" width="{EW}" height="{EH}" rx="12" fill="#e0ecff" stroke="{C_ESP}" stroke-width="2.5"/>')
text(EX + EW / 2, EY + 470, "ESP32", 26, "middle", C_ESP, "bold")
text(EX + EW / 2, EY + 492, "DevKit V1 / WROOM-32", 12, "middle", C_ESP)
text(EX + EW / 2, EY + 510, "Wi-Fi · NTP · N2YO HTTPS", 11, "middle", "#4b5563")
text(EX + EW / 2, EY + 528, "USB: flash + Serial 115200", 11, "middle", "#4b5563")
dot(600, EY); text(600, EY + 18, "VIN (5V)", 11, "middle")
dot(680, EY); text(680, EY + 18, "GND", 11, "middle")

# ---------------------------------------------------------------- TFT
TX, TY = 120, 230
box(TX, TY, 210, 270, "#fef3c7", C_TFT, "TFT ILI9341 2.8\"", "320×240 · SPI (VSPI)")
tft_pins = [("VCC", "3V3", 290), ("GND", "GND", 315), ("SCK", "GPIO18", 340), ("SDI/MOSI", "GPIO23", 365),
            ("SDO/MISO", "GPIO19", 390), ("CS", "GPIO15", 415), ("DC/RS", "GPIO2", 440), ("RESET", "GPIO4", 465)]
for p, g, y in tft_pins:
    pin_right(TX + 210, y, p)
    pin_left(EX, y, g)
    sig(TX + 210, EX, y, {"SCK": "SCK", "SDI/MOSI": "MOSI", "SDO/MISO": "MISO (optional)", "CS": "TFT_CS",
                          "DC/RS": "TFT_DC", "RESET": "TFT_RST", "VCC": "3V3", "GND": "GND"}[p],
        C_PWR if p == "VCC" else ("#1f2937" if p == "GND" else C_TFT))
# LED backlight pin
pin_left(TX, 300, "LED")
wire(TX, 300, TX - 30, 300, C_PWR)
netflag(TX - 30, 300, "3V3", C_PWR, "right")
text(TX + 105, TY + 262, "LED = backlight (tie to 3V3)", 10, "middle", "#4b5563")

# ---------------------------------------------------------------- Encoder
NX, NY = 120, 560
box(NX, NY, 210, 130, "#dcfce7", C_ENC, "Rotary encoder", "KY-040 (power from 3V3!)")
enc = [("CLK (A)", "GPIO34", 610, "ENC_A"), ("SW", "GPIO13", 640, "ENC_BTN"), ("DT (B)", "GPIO35", 670, "ENC_B")]
for p, g, y, n in enc:
    pin_right(NX + 210, y, p)
    pin_left(EX, y, g)
    sig(NX + 210, EX, y, n, C_ENC)
pin_left(NX, 610, "+")
wire(NX, 610, NX - 30, 610, C_PWR); netflag(NX - 30, 610, "3V3", C_PWR, "right")
pin_left(NX, 650, "GND")
wire(NX, 650, NX - 20, 650); gnd(NX - 20, 650)
# pull-ups on A/B (input-only pins)
dot(400, 610, C_ENC); resistor_v(400, 545, 610, "R1 10k*", C_ENC); netflag(400, 545, "3V3", C_PWR, "bottom")
dot(450, 670, C_ENC); resistor_v(450, 670, 725, "R2 10k*", C_ENC); netflag(450, 725, "3V3", C_PWR, "top")

# ---------------------------------------------------------------- Homing (optional)
HX, HY = 120, 790
box(HX, HY, 210, 135, "#f3f4f6", C_HOME, "Home switches", "(optional)", dashed=True)
for (p, g, y, n), rx in zip([("SW_AZ", "GPIO39", 835, "HOME_AZ"), ("SW_EL", "GPIO36", 865, "HOME_EL")], (400, 450)):
    pin_right(HX + 210, y, p)
    pin_left(EX, y, g)
    wire(HX + 210, y, EX, y, C_HOME, dashed=True)
    text(365, y - 5, n, 10, "middle", C_HOME, "bold")
    dot(rx, y, C_HOME)
    if rx == 400:
        resistor_v(rx, y - 55, y, "10k", C_HOME); netflag(rx, y - 55, "3V3", C_PWR, "bottom")
    else:
        resistor_v(rx, y, y + 50, "10k", C_HOME); netflag(rx, y + 50, "3V3", C_PWR, "top")
text(HX + 105, HY + 112, "NO microswitch → GND (active LOW)", 10, "middle", "#4b5563")
text(HX + 105, HY + 126, "enable with #define USE_HOMING 1", 10, "middle", "#4b5563")
text(HX + 105, HY + 98, "+100 nF to GND per input", 10, "middle", "#4b5563")

# ---------------------------------------------------------------- Drivers
def driver(DY, axis, step_g, dir_g, en_g, sy):
    DX = 960
    box(DX, DY, 190, 200, "#fce7f3", C_DRV, f"Stepper driver {axis}", "TMC2209 / A4988 / DRV8825")
    rows = [("STEP", step_g, f"STEP_{axis}"), ("DIR", dir_g, f"DIR_{axis}"), ("EN", en_g, f"EN_{axis}")]
    for i, (p, g, n) in enumerate(rows):
        y = sy + 30 * i
        pin_left(DX, y, p)
        pin_right(EX + EW, y, g)
        sig(EX + EW, DX, y, n, C_DRV)
    # EN pull-up keeps driver disabled while ESP32 boots (goes down to a 3V3 flag)
    ex = 820; ey = sy + 60
    dot(ex, ey, C_DRV)
    resistor_v(ex, ey, ey + 58, "10k", C_DRV)
    netflag(ex, ey + 58, "3V3", C_PWR, "top")
    # logic supply
    pin_left(DX, sy + 100, "VDD")
    wire(DX, sy + 100, DX - 30, sy + 100, C_PWR); netflag(DX - 30, sy + 100, "3V3", C_PWR, "right")
    pin_left(DX, sy + 130, "GND")
    wire(DX, sy + 130, DX - 20, sy + 130); gnd(DX - 20, sy + 130)
    # motor power on bottom edge
    BY = DY + 200
    dot(1050, BY); text(1050, BY - 10, "VMOT", 10, "middle", C_PWR, "bold")
    dot(1120, BY); text(1120, BY - 10, "GND", 10, "middle", "#1f2937", "bold")
    wire(1050, BY, 1050, BY + 40, C_PWR); netflag(1050, BY + 40, "+VMOT", C_PWR, "top")
    wire(1120, BY, 1120, BY + 20)
    dot(1050, BY + 20, C_PWR); dot(1120, BY + 20)
    cap_h(1050, 1120, BY + 20, "")
    gnd(1120, BY + 20)
    text(1170, BY + 26, "100 µF ≥35 V", 10, "start", "#374151")
    # coils to motor
    MX, MY = 1290, sy + 45
    for i, c in enumerate(["1A", "1B", "2A", "2B"]):
        y = sy + 30 * i
        pin_right(DX + 190, y, c)
        col = "#b91c1c" if c.startswith("1") else "#1d4ed8"
        wire(DX + 190, y, MX - 52, y, col)
    add(f'<circle cx="{MX}" cy="{MY}" r="55" fill="#ffffff" stroke="#111827" stroke-width="2.5"/>')
    text(MX, MY - 6, "M", 26, "middle", "#111827", "bold")
    text(MX, MY + 14, f"NEMA17 {axis}", 11, "middle")
    text(MX, MY + 30, "1.8°/step", 10, "middle", "#4b5563")
    text(MX, MY + 76, "coil A = 1A/1B · coil B = 2A/2B", 10, "middle", "#4b5563")

driver(250, "AZ", "GPIO26", "GPIO27", "GPIO14", 300)
driver(560, "EL", "GPIO33", "GPIO25", "GPIO32", 610)

# ---------------------------------------------------------------- notes
NY0 = 980
add(f'<rect x="20" y="{NY0}" width="{W-40}" height="150" rx="10" fill="#f9fafb" stroke="#d1d5db"/>')
text(36, NY0 + 24, "Notes", 14, "start", "#111827", "bold")
notes = [
    "1. ALL grounds are common: PSU −, buck GND, ESP32 GND, driver logic GND and driver motor GND.",
    "2. GPIO34/35/36/39 are input-only with no internal pull-ups → external 10 kΩ to 3V3 are mandatory. Never feed 5 V into any ESP32 pin.",
    "3. EN pull-ups keep the drivers disabled while the ESP32 boots (GPIO14 toggles at reset). Menu item \"EN active LOW\" must match your driver.",
    "4. Set the driver current limit (Vref) BEFORE connecting motors; never unplug a motor while the driver is powered.",
    "5. A4988/DRV8825: bridge RESET↔SLEEP. TMC2209 standalone: MS1/MS2 set microstepping. Recompute steps/deg after any change.",
    "6. * R1/R2 are usually already fitted on KY-040 modules — check before adding. GPIO2/GPIO15 are boot-strapping pins: keep TFT wired exactly as shown.",
]
for i, n in enumerate(notes):
    text(36, NY0 + 46 + 17 * i, n, 11.5, "start", "#374151")

svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">'
       + "".join(e for e in el if e) + "</svg>")
open(OUT, "w").write(svg)
print("wrote", OUT)
