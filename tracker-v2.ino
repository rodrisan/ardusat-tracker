/*
  ESP32 Satellite Tracker (N2YO) + TFT ILI9341 + AccelStepper + Rotary Encoder Menu
  - Runtime config (beamwidth, limits, steps/deg, backlash, motion profiles)
  - Save/Load to NVS (Preferences)
  - Encoder: ISR on A rising, read B for direction (no state table)
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <time.h>
#include <AccelStepper.h>
#include <Preferences.h>

// ====================== TFT ======================
#define TFT_CS    15
#define TFT_DC    2
#define TFT_RST   4
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// ====================== WIFI / API ======================
const char* ssid = "";
const char* password = "";
const char* apiKey = "";
const int   noradID = 25544;

float latitude  = 20.628914;
float longitude = -103.369780;
int   altitude  = 1600;

WiFiClientSecure wifi;
const char* serverAddress = "api.n2yo.com";
const int   serverPort = 443;
HttpClient client(wifi, serverAddress, serverPort);

String nombreSatelite = "";

// ====================== PINS (MOTORES) ======================
// Ajusta a tu hardware
#define DIR_AZ   27
#define STEP_AZ  26
#define EN_AZ    14

#define DIR_EL   25
#define STEP_EL  33
#define EN_EL    32

// ====================== PINS (ENCODER) ======================
// Si usas 34/35 (input-only) necesitas pullups externas en A/B
#define ENC_A    34
#define ENC_B    35
#define ENC_BTN  13   // si puedes, usa un GPIO normal con INPUT_PULLUP (no 34/35)

// ====================== HOMING (OPCIONAL) ======================
// Si no tienes switches todavía: deja USE_HOMING = 0
#define USE_HOMING 0
#if USE_HOMING
  #define HOME_AZ_PIN  39   // input-only ok, requiere pullup externa
  #define HOME_EL_PIN  36
  #define HOME_ACTIVE_LOW 1
  // Dirección hacia el switch: +1 o -1 (ajusta a tu mecánica)
  #define HOME_AZ_DIR  -1
  #define HOME_EL_DIR  -1
#endif

// ====================== TRACK STATE ======================
enum TrackState : uint8_t { IDLE=0, PREPASS=1, INPASS=2, PARKING=3, HOMING=4 };
TrackState trackState = IDLE;

// ====================== UI EVENTS ======================
enum UiEvent : uint8_t { EVT_NONE, EVT_UP, EVT_DOWN, EVT_SELECT, EVT_BACK };

// Declared here (before the first function) so Arduino's auto-generated prototypes can see them
enum MotionProfile : uint8_t { MP_TRACK, MP_PARK, MP_HOME };
enum ActionId : uint8_t { ACT_SAVE, ACT_DEFAULTS, ACT_EXIT, ACT_PARK_NOW, ACT_HOME_NOW };

// ====================== CONFIG STRUCT ======================
struct TrackerConfig {
  uint32_t magic   = 0x54524B52; // 'TRKR'
  uint16_t version = 1;

  // Antenna / deadband
  float beamwidthAzDeg = 20.0f;
  float beamwidthElDeg = 20.0f;
  float deadbandFactor = 0.5f;     // deadband = beamwidth * factor

  // Rate limit moves
  uint32_t minMoveIntervalMsAz = 800;
  uint32_t minMoveIntervalMsEl = 800;

  // Pass window
  uint32_t prepassSec = 10 * 60;

  // Limits
  float elMinDeg = -5.0f;      // seguridad
  float elMaxDeg = 90.0f;

  bool  azContinuous = true;
  float azMinDeg = 0.0f;
  float azMaxDeg = 360.0f;

  // Parking
  bool  parkWhenIdle = true;
  float parkAzDeg = 0.0f;
  float parkElDeg = 0.0f;

  // Steps/deg & backlash
  float stepsPerDegAz = 10.0f;
  float stepsPerDegEl = 10.0f;
  int32_t backlashAzSteps = 0;
  int32_t backlashElSteps = 0;

  // Enable polarity
  bool enableActiveLow = true;

  // Motion profiles (steps/s, steps/s^2)
  // Tracking (suave)
  float trackAzMaxSpeed = 800.0f;
  float trackAzAccel    = 600.0f;
  float trackElMaxSpeed = 800.0f;
  float trackElAccel    = 600.0f;

  // Parking (medio)
  float parkAzMaxSpeed  = 1200.0f;
  float parkAzAccel     = 900.0f;
  float parkElMaxSpeed  = 1200.0f;
  float parkElAccel     = 900.0f;

  // Homing (más rápido pero seguro)
  float homeAzMaxSpeed  = 2500.0f;
  float homeAzAccel     = 1500.0f;
  float homeElMaxSpeed  = 2500.0f;
  float homeElAccel     = 1500.0f;

  uint32_t crc = 0;
};

TrackerConfig cfg, prevCfg;

// ====================== CRC32 (simple) ======================
static inline uint32_t crc32_update(uint32_t crc, uint8_t data) {
  crc ^= data;
  for (int i = 0; i < 8; i++) crc = (crc >> 1) ^ (0xEDB88320u & (-(int)(crc & 1)));
  return crc;
}
uint32_t crc32_buf(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < len; i++) crc = crc32_update(crc, data[i]);
  return ~crc;
}
uint32_t computeCfgCrc(const TrackerConfig& c) {
  TrackerConfig tmp = c;
  tmp.crc = 0;
  return crc32_buf(reinterpret_cast<const uint8_t*>(&tmp), sizeof(tmp));
}
void setDefaults(TrackerConfig& c) {
  c = TrackerConfig();
  c.crc = computeCfgCrc(c);
}
bool validateCfg(const TrackerConfig& c) {
  if (c.magic != 0x54524B52 || c.version != 1) return false;
  if (c.beamwidthAzDeg < 1 || c.beamwidthAzDeg > 360) return false;
  if (c.beamwidthElDeg < 1 || c.beamwidthElDeg > 180) return false;
  if (c.deadbandFactor <= 0.0f || c.deadbandFactor > 1.0f) return false;
  if (c.stepsPerDegAz <= 0.01f || c.stepsPerDegAz > 100000.0f) return false;
  if (c.stepsPerDegEl <= 0.01f || c.stepsPerDegEl > 100000.0f) return false;
  if (c.elMinDeg < -30.0f || c.elMinDeg > 0.0f) return false;
  if (c.elMaxDeg < 10.0f || c.elMaxDeg > 120.0f) return false;
  if (c.elMinDeg >= c.elMaxDeg) return false;
  if (computeCfgCrc(c) != c.crc) return false;
  return true;
}

// ====================== CONFIG STORE (NVS) ======================
Preferences prefs;

bool loadConfig(TrackerConfig& out) {
  if (!prefs.begin("tracker", false)) return false;
  size_t len = prefs.getBytesLength("cfg");
  if (len != sizeof(TrackerConfig)) return false;
  TrackerConfig tmp;
  size_t got = prefs.getBytes("cfg", &tmp, sizeof(tmp));
  if (got != sizeof(tmp)) return false;
  if (!validateCfg(tmp)) return false;
  out = tmp;
  return true;
}

bool saveConfig(TrackerConfig& inout) {
  inout.magic = 0x54524B52;
  inout.version = 1;
  inout.crc = computeCfgCrc(inout);
  size_t put = prefs.putBytes("cfg", &inout, sizeof(inout));
  return put == sizeof(inout);
}

// ====================== STEPPERS ======================
AccelStepper stepperAz(AccelStepper::DRIVER, STEP_AZ, DIR_AZ);
AccelStepper stepperEl(AccelStepper::DRIVER, STEP_EL, DIR_EL);

// Movement control
unsigned long lastMoveMsAz = 0, lastMoveMsEl = 0;
int8_t lastDirAz = 0, lastDirEl = 0;

// ====================== PASS / TIMERS ======================
time_t tiempoInicioPase = 0;
time_t tiempoFinPase    = 0;

unsigned long lastPassMs = 0;
unsigned long lastPosMs  = 0;
unsigned long lastWiFiRetryMs = 0;

uint32_t passIntervalMs = 300000; // dynamic
uint32_t posIntervalMs  = 30000;  // dynamic

// Error counters
int erroresPosicion = 0;
int erroresPase = 0;
const int maxErrores = 10;

// ====================== HELPERS ======================
static inline float clampf(float x, float a, float b) { return (x < a) ? a : (x > b) ? b : x; }
static inline float wrap360(float a) { while (a < 0) a += 360.0f; while (a >= 360.0f) a -= 360.0f; return a; }
static inline float shortestDeltaDeg(float targetDeg, float currentDeg) {
  float d = wrap360(targetDeg) - wrap360(currentDeg);
  if (d > 180.0f)  d -= 360.0f;
  if (d < -180.0f) d += 360.0f;
  return d;
}
static inline long degToStepsAz(float deg) { return lroundf(deg * cfg.stepsPerDegAz); }
static inline long degToStepsEl(float deg) { return lroundf(deg * cfg.stepsPerDegEl); }
static inline float stepsToDegAz(long steps) { return (float)steps / cfg.stepsPerDegAz; }
static inline float stepsToDegEl(long steps) { return (float)steps / cfg.stepsPerDegEl; }

float currentAzDeg() { return stepsToDegAz(stepperAz.currentPosition()); }
float currentElDeg() { return stepsToDegEl(stepperEl.currentPosition()); }

void enableMotors(bool en) {
  if (en) {
    stepperAz.enableOutputs();
    stepperEl.enableOutputs();
  } else {
    stepperAz.disableOutputs();
    stepperEl.disableOutputs();
  }
}

// ====================== MOTION PROFILES ======================

void applyMotionProfile(MotionProfile p) {
  switch (p) {
    case MP_TRACK:
      stepperAz.setMaxSpeed(cfg.trackAzMaxSpeed);
      stepperAz.setAcceleration(cfg.trackAzAccel);
      stepperEl.setMaxSpeed(cfg.trackElMaxSpeed);
      stepperEl.setAcceleration(cfg.trackElAccel);
      break;
    case MP_PARK:
      stepperAz.setMaxSpeed(cfg.parkAzMaxSpeed);
      stepperAz.setAcceleration(cfg.parkAzAccel);
      stepperEl.setMaxSpeed(cfg.parkElMaxSpeed);
      stepperEl.setAcceleration(cfg.parkElAccel);
      break;
    case MP_HOME:
      stepperAz.setMaxSpeed(cfg.homeAzMaxSpeed);
      stepperAz.setAcceleration(cfg.homeAzAccel);
      stepperEl.setMaxSpeed(cfg.homeElMaxSpeed);
      stepperEl.setAcceleration(cfg.homeElAccel);
      break;
  }
}

// Apply profile by state
void applyProfileForState(TrackState st) {
  if (st == INPASS || st == PREPASS) applyMotionProfile(MP_TRACK);
  else if (st == PARKING) applyMotionProfile(MP_PARK);
  else if (st == HOMING) applyMotionProfile(MP_HOME);
}

// ====================== RESCALE when steps/deg changes ======================
void rescaleStepper(AccelStepper& st, float oldStepsPerDeg, float newStepsPerDeg) {
  if (oldStepsPerDeg <= 0.0f || newStepsPerDeg <= 0.0f) return;

  long curSteps = st.currentPosition();
  long tgtSteps = st.targetPosition();

  float curDeg = (float)curSteps / oldStepsPerDeg;
  float tgtDeg = (float)tgtSteps / oldStepsPerDeg;

  st.setCurrentPosition(lroundf(curDeg * newStepsPerDeg));
  st.moveTo(lroundf(tgtDeg * newStepsPerDeg));
}

// ====================== TRACK STATE UPDATE ======================
void updateTrackStateFromPass(time_t nowUtc) {
  bool hasPass = (tiempoInicioPase > 0 && tiempoFinPase > 0);

  if (!hasPass) {
    trackState = cfg.parkWhenIdle ? PARKING : IDLE;
    return;
  }

  if (nowUtc >= tiempoInicioPase && nowUtc <= tiempoFinPase) {
    trackState = INPASS;
  } else if (nowUtc < tiempoInicioPase && (uint32_t)(tiempoInicioPase - nowUtc) <= cfg.prepassSec) {
    trackState = PREPASS;
  } else {
    trackState = cfg.parkWhenIdle ? PARKING : IDLE;
  }
}

void updateIntervalsFromState() {
  // Llamadas a pase y posición (puedes ajustar)
  if (trackState == INPASS) {
    posIntervalMs  = 1000;
    passIntervalMs = 60000;
  } else if (trackState == PREPASS) {
    posIntervalMs  = 5000;
    passIntervalMs = 60000;
  } else {
    posIntervalMs  = 30000;
    passIntervalMs = 300000;
  }
}

// ====================== MOVE WITH DEADBAND + RATE LIMIT + BACKLASH ======================
void maybeMoveAz(float targetAzDeg) {
  unsigned long nowMs = millis();
  if ((nowMs - lastMoveMsAz) < cfg.minMoveIntervalMsAz) return;

  float deadband = cfg.beamwidthAzDeg * cfg.deadbandFactor;

  float cur = currentAzDeg();
  float deltaDeg = cfg.azContinuous ? shortestDeltaDeg(targetAzDeg, cur)
                                    : (clampf(targetAzDeg, cfg.azMinDeg, cfg.azMaxDeg) - cur);

  if (fabsf(deltaDeg) <= deadband) return;

  long deltaSteps = lroundf(deltaDeg * cfg.stepsPerDegAz);
  if (deltaSteps == 0) return;

  int8_t dir = (deltaSteps > 0) ? 1 : -1;
  if (cfg.backlashAzSteps > 0 && lastDirAz != 0 && dir != lastDirAz) {
    deltaSteps += dir * cfg.backlashAzSteps;
  }
  lastDirAz = dir;

  stepperAz.move(deltaSteps);
  lastMoveMsAz = nowMs;
}

void maybeMoveEl(float targetElDeg) {
  unsigned long nowMs = millis();
  if ((nowMs - lastMoveMsEl) < cfg.minMoveIntervalMsEl) return;

  float deadband = cfg.beamwidthElDeg * cfg.deadbandFactor;

  targetElDeg = clampf(targetElDeg, cfg.elMinDeg, cfg.elMaxDeg);

  float cur = currentElDeg();
  float deltaDeg = targetElDeg - cur;

  if (fabsf(deltaDeg) <= deadband) return;

  long deltaSteps = lroundf(deltaDeg * cfg.stepsPerDegEl);
  if (deltaSteps == 0) return;

  int8_t dir = (deltaSteps > 0) ? 1 : -1;
  if (cfg.backlashElSteps > 0 && lastDirEl != 0 && dir != lastDirEl) {
    deltaSteps += dir * cfg.backlashElSteps;
  }
  lastDirEl = dir;

  stepperEl.move(deltaSteps);
  lastMoveMsEl = nowMs;
}

void requestParkMove() {
  float tgtAz = cfg.parkAzDeg;
  float tgtEl = clampf(cfg.parkElDeg, cfg.elMinDeg, cfg.elMaxDeg);

  float curAz = currentAzDeg();
  float deltaAz = cfg.azContinuous ? shortestDeltaDeg(tgtAz, curAz)
                                   : (clampf(tgtAz, cfg.azMinDeg, cfg.azMaxDeg) - curAz);
  long stepsAz = lroundf(deltaAz * cfg.stepsPerDegAz);

  float curEl = currentElDeg();
  float deltaEl = tgtEl - curEl;
  long stepsEl = lroundf(deltaEl * cfg.stepsPerDegEl);

  if (stepsAz != 0) stepperAz.move(stepsAz);
  if (stepsEl != 0) stepperEl.move(stepsEl);
}

// ====================== API CALLS ======================
void obtenerPosicionActual() {
  if (erroresPosicion >= maxErrores) return;
  if (WiFi.status() != WL_CONNECTED) return;

  String path = "https://api.n2yo.com/rest/v1/satellite/positions/" +
                String(noradID) + "/" +
                String(latitude, 4) + "/" +
                String(longitude, 4) + "/" +
                String(altitude) + "/1/&apiKey=" + apiKey;

  client.get(path);
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  if (statusCode != 200) {
    erroresPosicion++;
    return;
  }
  erroresPosicion = 0;

  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, response);
  if (err) { erroresPosicion++; return; }

  nombreSatelite = String((const char*)(doc["info"]["satname"] | ""));
  float azimuth   = doc["positions"][0]["azimuth"]   | 0.0f;
  float elevation = doc["positions"][0]["elevation"] | -90.0f;

  // Solo mover en ventana
  if (trackState == PREPASS || trackState == INPASS) {
    enableMotors(true);
    applyProfileForState(trackState);
    maybeMoveAz(azimuth);
    maybeMoveEl(elevation);
  }

  // UI (pantalla principal)
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);

  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(10, 10);
  tft.println("Satellite Tracker");

  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(10, 40);
  tft.print("Az T: "); tft.print(azimuth, 1);
  tft.print("  C: ");  tft.println(wrap360(currentAzDeg()), 1);

  tft.setTextColor(ILI9341_ORANGE);
  tft.setCursor(10, 65);
  tft.print("El T: "); tft.print(elevation, 1);
  tft.print("  C: ");  tft.println(currentElDeg(), 1);

  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(10, 95);
  tft.print("Sat: ");
  tft.println(nombreSatelite.length() ? nombreSatelite : "-");

  time_t nowUtc = time(nullptr);
  updateTrackStateFromPass(nowUtc);
  updateIntervalsFromState();

  tft.setCursor(10, 130);
  tft.setTextColor(ILI9341_WHITE);
  if (trackState == INPASS) {
    int restante = (int)(tiempoFinPase - nowUtc);
    tft.setTextColor(ILI9341_GREEN);
    tft.print("IN PASS  R: "); tft.print(restante); tft.println("s");
  } else if (trackState == PREPASS) {
    int falta = (int)(tiempoInicioPase - nowUtc);
    tft.setTextColor(ILI9341_YELLOW);
    tft.print("PREPASS  T-"); tft.print(falta); tft.println("s");
  } else if (trackState == PARKING) {
    tft.setTextColor(ILI9341_CYAN);
    tft.println("PARKING");
  } else if (trackState == HOMING) {
    tft.setTextColor(ILI9341_CYAN);
    tft.println("HOMING");
  } else {
    tft.println("IDLE");
  }

  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(10, 200);
  tft.print("BW:"); tft.print(cfg.beamwidthAzDeg,0);
  tft.print("/");   tft.print(cfg.beamwidthElDeg,0);
  tft.print(" DB:");tft.print(cfg.deadbandFactor,2);
}

void actualizarPase() {
  if (erroresPase >= maxErrores) return;
  if (WiFi.status() != WL_CONNECTED) return;

  String path = "https://api.n2yo.com/rest/v1/satellite/radiopasses/" +
                String(noradID) + "/" +
                String(latitude, 4) + "/" +
                String(longitude, 4) + "/" +
                String(altitude) + "/1/0/&apiKey=" + apiKey;

  client.get(path);
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  if (statusCode != 200) { erroresPase++; return; }
  erroresPase = 0;

  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, response);
  if (err) { erroresPase++; return; }

  JsonArray passes = doc["passes"].as<JsonArray>();
  if (passes.isNull() || passes.size() == 0) {
    tiempoInicioPase = 0;
    tiempoFinPase = 0;
    return;
  }

  JsonObject pass = passes[0];
  tiempoInicioPase = (time_t)(pass["startUTC"] | 0);
  tiempoFinPase    = (time_t)(pass["endUTC"]   | 0);
}

// ====================== WIFI / NTP ======================
void connectWiFi(uint32_t timeoutMs=20000) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < timeoutMs) {
    delay(300);
  }
}

bool syncNTP(uint32_t timeoutMs=20000) {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  unsigned long t0 = millis();
  while (time(nullptr) < 100000 && (millis() - t0) < timeoutMs) delay(250);
  return time(nullptr) >= 100000;
}

// ====================== ENCODER (tu método) ======================
class EncoderInputSimple {
public:
  EncoderInputSimple(int pinA, int pinB, int pinBtn, bool invertDir=false, bool btnActiveLow=true)
  : a(pinA), b(pinB), btn(pinBtn), invert(invertDir), btnLow(btnActiveLow) {}

  void begin() {
    self = this;
    pinMode(a, INPUT);
    pinMode(b, INPUT);
    pinMode(btn, btnLow ? INPUT_PULLUP : INPUT);

    attachInterrupt(digitalPinToInterrupt(a), EncoderInputSimple::isrA, RISING);

    btnLast = readBtnPressed();
    btnStable = btnLast;
    btnLastChangeMs = millis();
  }

  UiEvent poll() {
    UiEvent ev = EVT_NONE;

    // encoder ticks
    int32_t t = consumeTicks();
    if (t > 0) ev = EVT_UP;
    else if (t < 0) ev = EVT_DOWN;

    // button click/long
    uint32_t now = millis();
    bool raw = readBtnPressed();

    if (raw != btnLast) {
      btnLast = raw;
      btnLastChangeMs = now;
    }

    if ((now - btnLastChangeMs) > 30 && raw != btnStable) {
      btnStable = raw;
      if (btnStable) {
        btnDownMs = now;
      } else {
        uint32_t held = now - btnDownMs;
        if (held >= 600) { if (ev == EVT_NONE) ev = EVT_BACK; }
        else { if (ev == EVT_NONE) ev = EVT_SELECT; }
      }
    }

    return ev;
  }

private:
  int a,b,btn;
  bool invert;
  bool btnLow;

  volatile int32_t ticks = 0;
  volatile uint32_t lastIsrUs = 0;

  bool btnLast=false, btnStable=false;
  uint32_t btnLastChangeMs=0, btnDownMs=0;

  static EncoderInputSimple* self;
  static void isrA();  // defined outside the class: IRAM_ATTR on an in-class body fails to link

  int32_t consumeTicks() {
    noInterrupts();
    int32_t t = ticks;
    ticks = 0;
    interrupts();
    return t;
  }

  bool readBtnPressed() const {
    bool v = digitalRead(btn);
    return btnLow ? (v == LOW) : (v == HIGH);
  }
};
EncoderInputSimple* EncoderInputSimple::self = nullptr;
void IRAM_ATTR EncoderInputSimple::isrA() {
  if (!self) return;

  uint32_t nowUs = (uint32_t)micros();
  // debounce ISR simple
  if (nowUs - self->lastIsrUs < 250) return; // 250us
  self->lastIsrUs = nowUs;

  bool bState = digitalRead(self->b);
  int dir = bState ? -1 : +1;
  if (self->invert) dir = -dir;
  self->ticks += dir;
}
EncoderInputSimple encoder(ENC_A, ENC_B, ENC_BTN, false, true);

// ====================== MENU ======================
enum ItemType : uint8_t { IT_FLOAT, IT_INT32, IT_UINT32, IT_BOOL, IT_ACTION };

struct MenuItem {
  const char* label;
  ItemType type;
  void* ptr;

  float fStep, fMin, fMax;
  int32_t iStep, iMin, iMax;
  uint32_t uStep, uMin, uMax;

  const char* unit;
  ActionId action;
};

bool menuOpen = false;
bool menuEdit = false;
int  menuIndex = 0;

// flags from actions
bool flagSave=false, flagDefaults=false, flagExit=false, flagParkNow=false, flagHomeNow=false;

void doAction(ActionId a) {
  if (a == ACT_SAVE) flagSave = true;
  if (a == ACT_DEFAULTS) flagDefaults = true;
  if (a == ACT_EXIT) flagExit = true;
  if (a == ACT_PARK_NOW) flagParkNow = true;
  if (a == ACT_HOME_NOW) flagHomeNow = true;
}

MenuItem menuItems[] = {
  // Beamwidth / deadband
  {"BW Az", IT_FLOAT, &cfg.beamwidthAzDeg,  1.0,  1.0, 120.0, 0,0,0, 0,0,0, "deg", ACT_EXIT},
  {"BW El", IT_FLOAT, &cfg.beamwidthElDeg,  1.0,  1.0, 120.0, 0,0,0, 0,0,0, "deg", ACT_EXIT},
  {"Deadband", IT_FLOAT, &cfg.deadbandFactor, 0.05, 0.10, 1.00, 0,0,0, 0,0,0, "", ACT_EXIT},

  // Steps/deg + backlash
  {"Steps/deg AZ", IT_FLOAT, &cfg.stepsPerDegAz, 0.10, 0.01, 50000.0, 0,0,0, 0,0,0, "", ACT_EXIT},
  {"Steps/deg EL", IT_FLOAT, &cfg.stepsPerDegEl, 0.10, 0.01, 50000.0, 0,0,0, 0,0,0, "", ACT_EXIT},
  {"Backlash AZ", IT_INT32, &cfg.backlashAzSteps, 0,0,0, 10, 0, 50000, 0,0,0, "steps", ACT_EXIT},
  {"Backlash EL", IT_INT32, &cfg.backlashElSteps, 0,0,0, 10, 0, 50000, 0,0,0, "steps", ACT_EXIT},

  // Limits
  {"EL Min", IT_FLOAT, &cfg.elMinDeg, 0.5, -30.0, 0.0, 0,0,0, 0,0,0, "deg", ACT_EXIT},
  {"EL Max", IT_FLOAT, &cfg.elMaxDeg, 1.0, 10.0, 120.0, 0,0,0, 0,0,0, "deg", ACT_EXIT},
  {"Prepass", IT_UINT32, &cfg.prepassSec, 0,0,0, 0,0,0, 60, 60, 3600, "s", ACT_EXIT},

  // Rate limit
  {"MinMove AZ", IT_UINT32, &cfg.minMoveIntervalMsAz, 0,0,0, 0,0,0, 50, 0, 5000, "ms", ACT_EXIT},
  {"MinMove EL", IT_UINT32, &cfg.minMoveIntervalMsEl, 0,0,0, 0,0,0, 50, 0, 5000, "ms", ACT_EXIT},

  // Enable polarity
  {"EN active LOW", IT_BOOL, &cfg.enableActiveLow, 0,0,0, 0,0,0, 0,0,0, "", ACT_EXIT},

  // Motion profiles (TRACK)
  {"TR Az MaxSpd", IT_FLOAT, &cfg.trackAzMaxSpeed, 50.0, 50.0, 50000.0, 0,0,0, 0,0,0, "st/s", ACT_EXIT},
  {"TR Az Accel",  IT_FLOAT, &cfg.trackAzAccel,    50.0, 50.0, 50000.0, 0,0,0, 0,0,0, "st/s2", ACT_EXIT},
  {"TR El MaxSpd", IT_FLOAT, &cfg.trackElMaxSpeed, 50.0, 50.0, 50000.0, 0,0,0, 0,0,0, "st/s", ACT_EXIT},
  {"TR El Accel",  IT_FLOAT, &cfg.trackElAccel,    50.0, 50.0, 50000.0, 0,0,0, 0,0,0, "st/s2", ACT_EXIT},

  // Motion profiles (HOME)
  {"HM Az MaxSpd", IT_FLOAT, &cfg.homeAzMaxSpeed, 100.0, 50.0, 80000.0, 0,0,0, 0,0,0, "st/s", ACT_EXIT},
  {"HM Az Accel",  IT_FLOAT, &cfg.homeAzAccel,    100.0, 50.0, 80000.0, 0,0,0, 0,0,0, "st/s2", ACT_EXIT},
  {"HM El MaxSpd", IT_FLOAT, &cfg.homeElMaxSpeed, 100.0, 50.0, 80000.0, 0,0,0, 0,0,0, "st/s", ACT_EXIT},
  {"HM El Accel",  IT_FLOAT, &cfg.homeElAccel,    100.0, 50.0, 80000.0, 0,0,0, 0,0,0, "st/s2", ACT_EXIT},

  // Motion profiles (PARK)
  {"PK Az MaxSpd", IT_FLOAT, &cfg.parkAzMaxSpeed, 50.0, 50.0, 80000.0, 0,0,0, 0,0,0, "st/s", ACT_EXIT},
  {"PK Az Accel",  IT_FLOAT, &cfg.parkAzAccel,    50.0, 50.0, 80000.0, 0,0,0, 0,0,0, "st/s2", ACT_EXIT},
  {"PK El MaxSpd", IT_FLOAT, &cfg.parkElMaxSpeed, 50.0, 50.0, 80000.0, 0,0,0, 0,0,0, "st/s", ACT_EXIT},
  {"PK El Accel",  IT_FLOAT, &cfg.parkElAccel,    50.0, 50.0, 80000.0, 0,0,0, 0,0,0, "st/s2", ACT_EXIT},

  // Actions
  {"PARK NOW", IT_ACTION, nullptr, 0,0,0, 0,0,0, 0,0,0, "", ACT_PARK_NOW},
  {"HOME NOW", IT_ACTION, nullptr, 0,0,0, 0,0,0, 0,0,0, "", ACT_HOME_NOW},
  {"SAVE",     IT_ACTION, nullptr, 0,0,0, 0,0,0, 0,0,0, "", ACT_SAVE},
  {"DEFAULTS", IT_ACTION, nullptr, 0,0,0, 0,0,0, 0,0,0, "", ACT_DEFAULTS},
  {"EXIT",     IT_ACTION, nullptr, 0,0,0, 0,0,0, 0,0,0, "", ACT_EXIT},
};
const int menuCount = sizeof(menuItems)/sizeof(menuItems[0]);

void menuRender() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_CYAN);
  tft.print("MENU ");
  tft.setTextColor(menuEdit ? ILI9341_YELLOW : ILI9341_WHITE);
  tft.println(menuEdit ? "[EDIT]" : "[NAV]");

  // scroll window
  const int visibleLines = 8;
  int start = menuIndex - visibleLines/2;
  if (start < 0) start = 0;
  if (start > menuCount - visibleLines) start = max(0, menuCount - visibleLines);

  int y = 40;
  for (int i = 0; i < visibleLines; i++) {
    int idx = start + i;
    if (idx >= menuCount) break;

    MenuItem& it = menuItems[idx];
    bool sel = (idx == menuIndex);

    tft.setCursor(10, y);
    tft.setTextColor(sel ? ILI9341_GREEN : ILI9341_WHITE);
    tft.print(sel ? "> " : "  ");
    tft.print(it.label);
    tft.print(": ");

    tft.setTextColor(sel ? ILI9341_YELLOW : ILI9341_LIGHTGREY);

    if (it.type == IT_FLOAT) {
      tft.print(*(float*)it.ptr, 2);
      if (it.unit && it.unit[0]) { tft.print(" "); tft.print(it.unit); }
    } else if (it.type == IT_INT32) {
      tft.print(*(int32_t*)it.ptr);
      if (it.unit && it.unit[0]) { tft.print(" "); tft.print(it.unit); }
    } else if (it.type == IT_UINT32) {
      tft.print(*(uint32_t*)it.ptr);
      if (it.unit && it.unit[0]) { tft.print(" "); tft.print(it.unit); }
    } else if (it.type == IT_BOOL) {
      tft.print(*(bool*)it.ptr ? "ON" : "OFF");
    } else {
      tft.print("[RUN]");
    }

    y += 22;
  }

  tft.setTextColor(ILI9341_DARKGREY);
  tft.setCursor(10, 220);
  tft.println("Click:Select  Hold:Back");
}

bool menuHandle(UiEvent e) {
  bool changed = false;
  if (!menuOpen) return false;

  if (e == EVT_UP) {
    if (menuEdit) {
      MenuItem& it = menuItems[menuIndex];
      if (it.type == IT_FLOAT) {
        float& v = *(float*)it.ptr;
        v = clampf(v + it.fStep, it.fMin, it.fMax);
        changed = true;
      } else if (it.type == IT_INT32) {
        int32_t& v = *(int32_t*)it.ptr;
        int64_t nv = (int64_t)v + it.iStep;
        if (nv < it.iMin) nv = it.iMin;
        if (nv > it.iMax) nv = it.iMax;
        v = (int32_t)nv;
        changed = true;
      } else if (it.type == IT_UINT32) {
        uint32_t& v = *(uint32_t*)it.ptr;
        int64_t nv = (int64_t)v + (int64_t)it.uStep;
        if (nv < (int64_t)it.uMin) nv = it.uMin;
        if (nv > (int64_t)it.uMax) nv = it.uMax;
        v = (uint32_t)nv;
        changed = true;
      } else if (it.type == IT_BOOL) {
        bool& v = *(bool*)it.ptr;
        v = !v;
        changed = true;
      }
    } else {
      if (menuIndex > 0) menuIndex--;
    }
  }

  if (e == EVT_DOWN) {
    if (menuEdit) {
      MenuItem& it = menuItems[menuIndex];
      if (it.type == IT_FLOAT) {
        float& v = *(float*)it.ptr;
        v = clampf(v - it.fStep, it.fMin, it.fMax);
        changed = true;
      } else if (it.type == IT_INT32) {
        int32_t& v = *(int32_t*)it.ptr;
        int64_t nv = (int64_t)v - it.iStep;
        if (nv < it.iMin) nv = it.iMin;
        if (nv > it.iMax) nv = it.iMax;
        v = (int32_t)nv;
        changed = true;
      } else if (it.type == IT_UINT32) {
        uint32_t& v = *(uint32_t*)it.ptr;
        int64_t nv = (int64_t)v - (int64_t)it.uStep;
        if (nv < (int64_t)it.uMin) nv = it.uMin;
        if (nv > (int64_t)it.uMax) nv = it.uMax;
        v = (uint32_t)nv;
        changed = true;
      } else if (it.type == IT_BOOL) {
        bool& v = *(bool*)it.ptr;
        v = !v;
        changed = true;
      }
    } else {
      if (menuIndex < menuCount-1) menuIndex++;
    }
  }

  if (e == EVT_SELECT) {
    MenuItem& it = menuItems[menuIndex];
    if (it.type == IT_ACTION) {
      doAction(it.action);
    } else {
      menuEdit = !menuEdit;
    }
  }

  if (e == EVT_BACK) {
    if (menuEdit) menuEdit = false;
    else { menuOpen = false; } // close menu
  }

  menuRender();
  return changed;
}

// ====================== APPLY CONFIG CHANGES ======================
void applyConfigChanges(const TrackerConfig& oldC, const TrackerConfig& newC) {
  // 1) enable polarity
  stepperAz.setPinsInverted(false, false, newC.enableActiveLow);
  stepperEl.setPinsInverted(false, false, newC.enableActiveLow);

  // 2) steps/deg change => rescale positions/targets to preserve angle
  if (oldC.stepsPerDegAz != newC.stepsPerDegAz) {
    rescaleStepper(stepperAz, oldC.stepsPerDegAz, newC.stepsPerDegAz);
  }
  if (oldC.stepsPerDegEl != newC.stepsPerDegEl) {
    rescaleStepper(stepperEl, oldC.stepsPerDegEl, newC.stepsPerDegEl);
  }

  // 3) clamp parkEl within new limits
  // (cfg already changed, but ensure safe)
  (void)oldC;
  applyProfileForState(trackState);
}

// ====================== HOMING STATE MACHINE (optional) ======================
#if USE_HOMING
bool isHomeActive(int pin) {
  int v = digitalRead(pin);
  return HOME_ACTIVE_LOW ? (v == LOW) : (v == HIGH);
}

struct HomingSM {
  bool active=false;
  uint8_t phase=0;
  unsigned long startMs=0;
  unsigned long phaseStartMs=0;
  long backoffSteps=200;

  void start() {
    active=true; phase=0; startMs=millis(); phaseStartMs=millis();
    trackState = HOMING;
    enableMotors(true);
    applyMotionProfile(MP_HOME);
  }

  void stop(bool ok) {
    (void)ok;
    active=false;
    trackState = cfg.parkWhenIdle ? PARKING : IDLE;
    applyProfileForState(trackState);
  }

  void tick() {
    if (!active) return;
    if (millis() - startMs > 30000) { stop(false); return; }

    // Phase 0: AZ toward switch using runSpeed
    if (phase == 0) {
      stepperAz.setSpeed((float)HOME_AZ_DIR * min(cfg.homeAzMaxSpeed, 1200.0f));
      if (!isHomeActive(HOME_AZ_PIN)) {
        stepperAz.runSpeed();
        return;
      }
      stepperAz.setCurrentPosition(0);
      stepperAz.move(-HOME_AZ_DIR * backoffSteps);
      phase = 1; phaseStartMs = millis();
      return;
    }

    // Phase 1: AZ backoff
    if (phase == 1) {
      if (stepperAz.distanceToGo() != 0) { stepperAz.run(); return; }
      phase = 2; phaseStartMs = millis();
      return;
    }

    // Phase 2: EL toward switch
    if (phase == 2) {
      stepperEl.setSpeed((float)HOME_EL_DIR * min(cfg.homeElMaxSpeed, 1200.0f));
      if (!isHomeActive(HOME_EL_PIN)) {
        stepperEl.runSpeed();
        return;
      }
      stepperEl.setCurrentPosition(0);
      stepperEl.move(-HOME_EL_DIR * backoffSteps);
      phase = 3; phaseStartMs = millis();
      return;
    }

    // Phase 3: EL backoff
    if (phase == 3) {
      if (stepperEl.distanceToGo() != 0) { stepperEl.run(); return; }
      stop(true);
      return;
    }
  }
};
HomingSM homing;
#endif

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 10);
  tft.println("Booting...");

  // Load config
  if (!loadConfig(cfg)) setDefaults(cfg);
  prevCfg = cfg;

  // Secure client (como tu setup)
  wifi.setInsecure();

  // WiFi + NTP
  connectWiFi();
  syncNTP();

  // Stepper init
  stepperAz.setEnablePin(EN_AZ);
  stepperEl.setEnablePin(EN_EL);
  stepperAz.setPinsInverted(false, false, cfg.enableActiveLow);
  stepperEl.setPinsInverted(false, false, cfg.enableActiveLow);

  enableMotors(false);
  applyProfileForState(trackState);

#if USE_HOMING
  pinMode(HOME_AZ_PIN, INPUT);
  pinMode(HOME_EL_PIN, INPUT);
#endif

  // Encoder init
  encoder.begin();

  // Force initial API fetch sooner
  lastPassMs = 0;
  lastPosMs  = 0;

  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_GREEN);
  tft.println("Ready");
  delay(500);
}

// ====================== LOOP ======================
void loop() {
  // Run steppers always
  stepperAz.run();
  stepperEl.run();

  // WiFi retry
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWiFiRetryMs > 10000) {
      lastWiFiRetryMs = millis();
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  }

  // Encoder UI
  UiEvent e = encoder.poll();

  // Open menu with click (EVT_SELECT) — si prefieres abrir con long press, cambia aquí
  if (!menuOpen && e == EVT_SELECT) {
    menuOpen = true;
    menuEdit = false;
    menuRender();
  }

  // If menu open: handle menu and pause tracking actions
  if (menuOpen) {
    bool changed = menuHandle(e);

    if (changed) {
      // apply changes live
      applyConfigChanges(prevCfg, cfg);
      prevCfg = cfg;
    }

    // handle actions
    if (flagDefaults) {
      flagDefaults = false;
      setDefaults(cfg);
      applyConfigChanges(prevCfg, cfg);
      prevCfg = cfg;
      menuRender();
    }

    if (flagSave) {
      flagSave = false;
      saveConfig(cfg);
      menuRender();
    }

    if (flagParkNow) {
      flagParkNow = false;
      trackState = PARKING;
      applyProfileForState(trackState);
      enableMotors(true);
      requestParkMove();
      menuRender();
    }

#if USE_HOMING
    if (flagHomeNow) {
      flagHomeNow = false;
      homing.start();
      menuRender();
    }
#else
    if (flagHomeNow) {
      // sin switches: solo ignora por ahora
      flagHomeNow = false;
      menuRender();
    }
#endif

    if (flagExit) {
      flagExit = false;
      menuOpen = false;
    }

    delay(1);
    return; // pausa tracking mientras editas
  }

  // Homing tick (si existe)
#if USE_HOMING
  if (trackState == HOMING) {
    homing.tick();
    delay(1);
    return;
  }
#endif

  // Update state from pass + intervals
  time_t nowUtc = time(nullptr);
  updateTrackStateFromPass(nowUtc);
  updateIntervalsFromState();
  applyProfileForState(trackState);

  // Parking behavior
  if (trackState == PARKING) {
    enableMotors(true);
    // si no hay movimiento, manda park
    if (stepperAz.distanceToGo() == 0 && stepperEl.distanceToGo() == 0) {
      requestParkMove();
    }

    // si ya llegó cerca, apaga motores y pasa a IDLE
    float azErr = fabsf(shortestDeltaDeg(cfg.parkAzDeg, currentAzDeg()));
    float elErr = fabsf(clampf(cfg.parkElDeg, cfg.elMinDeg, cfg.elMaxDeg) - currentElDeg());
    if (azErr < 1.0f && elErr < 1.0f && stepperAz.distanceToGo() == 0 && stepperEl.distanceToGo() == 0) {
      enableMotors(false);
      trackState = IDLE;
    }
  } else if (trackState == IDLE) {
    enableMotors(false);
  } else {
    // PREPASS/INPASS
    enableMotors(true);
  }

  // API calls (timed)
  unsigned long ms = millis();
  if (ms - lastPassMs >= passIntervalMs) {
    actualizarPase();
    lastPassMs = ms;
  }

  if (ms - lastPosMs >= posIntervalMs) {
    obtenerPosicionActual();
    lastPosMs = ms;
  }

  delay(1);
}
