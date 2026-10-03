// Zielsystem: ESP32-2432S028R ("Cheap Yellow Display").
//
// Laeuft auch OHNE angeschlossenen Sensor. Wird beim Start keiner gefunden,
// schaltet das Geraet in den Demobetrieb: ein synthetisches Signal laeuft
// durch dieselbe Messkette wie spaeter der echte Schall. Damit lassen sich
// Display, Rechenkette und Anzeige auf echter Hardware pruefen, bevor der
// BMP581 eintrifft.
//
// Offen (Phase 2): der eigentliche Sensortreiber. Die Erkennung unten liest
// nur die Chip-ID -- dafuer braucht es keine Herstellerbibliothek.

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>

#include "../include/config.h"
#include "bmp581.h"
#include "meter.h"

namespace {

TFT_eSPI tft;
vu::Meter g_meter;
Bmp581 g_sensor;

float g_fs = cfg::kFsNominal;
int g_backlight = cfg::kBacklightDefault;  // Prozent

enum class Screen { Live, Calib };
Screen g_screen = Screen::Live;

bool g_demoMode = true;        // kein Sensor gefunden -> Testsignal
bool g_fsError = false;        // gemessene Abtastrate zu niedrig fuers Band
uint32_t g_overruns = 0;       // FIFO lief voll, Messwerte fehlen

// Temperatur wird im Sensor-Task gelesen und hier abgelegt. Der Anzeige-Task
// darf den I2C-Bus nicht selbst anfassen -- zwei Tasks auf demselben Bus ohne
// Absicherung geben frueher oder spaeter vertauschte Register.
volatile float g_tempC = 0.0f;

// Bezugsdruck fuer den Kalibrierschirm. Langer Tastendruck setzt ihn auf den
// aktuellen Wert; angezeigt wird danach die Differenz. Damit braucht weder der
// Stockwerk- noch der Wassersaeulentest eine Nebenrechnung.
float g_refPa = 0.0f;
bool g_refSet = false;

// Luftdruckgradient nahe dem Boden: rund 12 Pa je Meter Hoehe.
constexpr float kPaPerMeter = 12.0f;
int g_sensorAddr = -1;   // -1 = kein Sensor gefunden, Demobetrieb
uint32_t g_samples = 0;  // verarbeitete Messwerte, fuer die Statuszeile

// ------------------------------------------------- Hintergrundbeleuchtung

// Helligkeit in Prozent. Ueber PWM, damit sie sich regeln laesst.
//
// Sich auf TFT_eSPI zu verlassen reicht nicht: die Bibliothek schaltet den Pin
// hoechstens einmal beim init() ein. Alles, was danach den Pin anfasst --
// etwa ein Wire.begin() auf demselben GPIO -- macht das wieder zunichte.
// Deshalb hier explizit und nach allen anderen Initialisierungen.
void setBacklight(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    g_backlight = percent;
    const uint32_t duty = (percent * ((1u << cfg::kBacklightBits) - 1)) / 100;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    // Arduino-Core 3.x hat die LEDC-Schnittstelle vereinfacht.
    ledcAttach(cfg::kBacklightPin, cfg::kBacklightPwmHz, cfg::kBacklightBits);
    ledcWrite(cfg::kBacklightPin, duty);
#else
    ledcSetup(cfg::kBacklightPwmCh, cfg::kBacklightPwmHz, cfg::kBacklightBits);
    ledcAttachPin(cfg::kBacklightPin, cfg::kBacklightPwmCh);
    ledcWrite(cfg::kBacklightPwmCh, duty);
#endif
}

// ------------------------------------------------------------------ Sensor

// Misst die tatsaechliche Abtastrate des Sensors.
//
// Der BMP581 taktet im Dauerbetrieb mit einem internen RC-Oszillator; die
// Rate haengt vom Oversampling ab und weicht vom Nennwert ab. Sie geht 1:1 in
// die Frequenzanzeige ein -- 5 % Fehler bei 40 Hz sind 2 Hz. Deshalb wird sie
// gemessen statt angenommen, und daraus werden Filter und FFT-Frequenzachse
// ausgelegt.
//
// Gezaehlt wird gegen die Uhr des ESP32, nicht gegen die des Sensors.
float measureSampleRate(float seconds) {
    float scratch[bmp581::kFifoMaxFrames];

    // Erst den FIFO leeren, damit angesammelte Werte nicht mitgezaehlt werden.
    g_sensor.readFifo(scratch, bmp581::kFifoMaxFrames);

    const uint32_t t0 = micros();
    uint32_t counted = 0;
    uint32_t overruns = 0;

    while ((micros() - t0) < static_cast<uint32_t>(seconds * 1e6f)) {
        const int n = g_sensor.readFifo(scratch, bmp581::kFifoMaxFrames);
        if (n > 0) {
            counted += static_cast<uint32_t>(n);
            if (g_sensor.overrun()) ++overruns;
        }
        delay(cfg::kFifoReadIntervalMs);
    }
    const uint32_t dtUs = micros() - t0;

    if (overruns > 0) {
        // Bei Ueberlauf fehlen Werte, die Rate kaeme zu niedrig heraus.
        Serial.printf("Abtastratenmessung: %lu Ueberlaeufe, Ergebnis unsicher\n",
                      static_cast<unsigned long>(overruns));
    }
    if (counted == 0 || dtUs == 0) return 0.0f;
    return static_cast<float>(counted) * 1e6f / static_cast<float>(dtUs);
}

// ----------------------------------------------------------- Demosignal

// Sinus mit linearem Frequenzsweep, auf Umgebungsdruck aufgesetzt. Die
// Momentanphase wird fortlaufend integriert, sonst springt das Signal bei
// jedem Frequenzwechsel und die FFT sieht einen Knacks statt eines Tons.
float demoSample(float dt) {
    static float phase = 0.0f;
    static float t = 0.0f;
    t += dt;
    if (t > cfg::kDemoSweepS) t -= cfg::kDemoSweepS;

    const float k = t / cfg::kDemoSweepS;
    const float f = cfg::kDemoFreqLo + k * (cfg::kDemoFreqHi - cfg::kDemoFreqLo);
    phase += 2.0f * PI * f * dt;
    if (phase > 2.0f * PI) phase -= 2.0f * PI;

    const float amp = vu::paFromSpl(cfg::kDemoSpl) * sqrtf(2.0f);
    return cfg::kDemoAmbientPa + amp * sinf(phase);
}

// -------------------------------------------------------------- Anzeige

// Eigene Pruefung statt isfinite(): das ist in <math.h> ein Makro und in
// <cmath> eine Funktion in std, je nach Toolchain auch beides. Der Vergleich
// hier kommt ohne beides aus.
inline bool plausible(float v) {
    return v == v && v > -1e30f && v < 1e30f;  // weder NaN noch unendlich
}

constexpr uint16_t kBg = TFT_BLACK;
constexpr uint16_t kFg = TFT_WHITE;
constexpr uint16_t kDim = 0x7BEF;  // mittleres Grau
constexpr uint16_t kAccent = TFT_CYAN;
constexpr uint16_t kWarn = TFT_ORANGE;

// Layout. Aus der Bildschirmgroesse abgeleitet statt aus geratenen Zahlen:
// die Statuszeile hing vorher unter dem Spektrum und wurde bei jedem Refresh
// um vier Pixel ueberschrieben.
constexpr int kScreenW = 320;
constexpr int kScreenH = 240;
constexpr int kFontSmallH = 16;  // Zeilenhoehe von TFT_eSPI-Font 2

constexpr int kStatusY = kScreenH - kFontSmallH;  // Statuszeile ganz unten
constexpr int kSpecGap = 2;                       // Luft darueber
constexpr int kSpecX = 8;
constexpr int kSpecW = 304;
constexpr int kSpecY = 150;                       // Oberkante des Spektrums
constexpr int kSpecH = kStatusY - kSpecGap - kSpecY;

static_assert(kSpecY + kSpecH + kSpecGap <= kStatusY,
              "Spektrum ragt in die Statuszeile");
static_assert(kStatusY + kFontSmallH <= kScreenH,
              "Statuszeile ragt ueber den unteren Bildrand");
static_assert(kSpecX + kSpecW <= kScreenW,
              "Spektrum ragt ueber den rechten Bildrand");
static_assert(kSpecH > 0, "Kein Platz fuer das Spektrum");

void drawStatic() {
    tft.fillScreen(kBg);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(kDim, kBg);
    tft.drawString("dB SPL", 232, 64, 2);
    tft.drawFastHLine(0, 140, 320, kDim);
    tft.drawFastHLine(kSpecX, kSpecY + kSpecH, kSpecW, kDim);
}

void drawSpectrum(const vu::Meter& m) {
    if (!m.spectrumReady()) return;
    const vu::Spectrum& sp = m.spectrum();
    const float* mags = m.magnitudes();
    const vu::BandLimits& bl = vu::bandLimits(m.band());

    // Ein Balken je Pixelspalte, logarithmisch ueber der Frequenz -- so
    // bekommt der Bereich unter 30 Hz genug Platz.
    const float logLo = log10f(bl.lo), logHi = log10f(bl.hi);
    for (int px = 0; px < kSpecW; ++px) {
        const float f = powf(10.0f, logLo + (logHi - logLo) * px / kSpecW);
        int k = static_cast<int>(f / sp.binHz() + 0.5f);
        if (k < 1) k = 1;
        if (k > sp.magCount() - 1) k = sp.magCount() - 1;

        // Bin-Betrag in einen Pegel umrechnen und auf 100..170 dB abbilden
        const float pa = 2.0f * mags[k] / (sp.size() * 0.5f) / sqrtf(2.0f);
        const float db = vu::splFromPa(pa);
        int h = static_cast<int>((db - 100.0f) / 70.0f * kSpecH);
        if (h < 0) h = 0;
        if (h > kSpecH) h = kSpecH;

        const int x = kSpecX + px;
        tft.drawFastVLine(x, kSpecY, kSpecH - h, kBg);
        if (h > 0) tft.drawFastVLine(x, kSpecY + kSpecH - h, h, kAccent);
    }
}

void drawReadings(const vu::Report& r) {
    char buf[48];

    // Grosser Pegelwert
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(kFg, kBg);
    // Font 7 ist eine Sieben-Segment-Schrift und kennt nur Ziffern, Punkt,
    // Doppelpunkt, Minus und Leerzeichen. Vor dem ersten eingespeisten
    // Messwert ist der Pegel -inf; ungefiltert stuenden dort Fragmente.
    float level = r.splSlow;
    if (!plausible(level) || level < 0.0f) level = 0.0f;
    snprintf(buf, sizeof(buf), "%5.1f", level);
    tft.drawString(buf, 228, 30, 7);

    // Dominante Frequenz
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(kAccent, kBg);
    snprintf(buf, sizeof(buf), "%6.2f Hz", r.f0);
    tft.drawString(buf, 8, 106, 4);

    // Spitzenwerte
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(kDim, kBg);
    const float maxRms = plausible(r.splMaxRms) ? r.splMaxRms : 0.0f;
    const float peak = plausible(r.splPeak) ? r.splPeak : 0.0f;
    snprintf(buf, sizeof(buf), "max %5.1f  peak %5.1f", maxRms, peak);
    tft.drawString(buf, 312, 112, 2);

    // Statuszeile
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor((r.pressurePlausible && g_overruns == 0) ? kDim : kWarn,
                     kBg);
    char mode[20];
    if (!g_demoMode) {
        if (g_overruns > 0) {
            // Nicht verschweigen: bei Ueberlauf fehlen Messwerte, der
            // angezeigte Pegel ist dann zu niedrig.
            snprintf(mode, sizeof(mode), "BMP581 OVR%lu",
                     static_cast<unsigned long>(g_overruns));
        } else {
            snprintf(mode, sizeof(mode), "BMP581 0x%02X", g_sensor.address());
        }
    } else {
        snprintf(mode, sizeof(mode), "DEMO");
    }
    snprintf(buf, sizeof(buf), "%s Hz  %.0f/s  %s  %d%%", r.band, g_fs, mode,
             g_backlight);
    tft.drawString(buf, 8, kStatusY, 2);
}

// Kalibrierschirm. Zeigt den Absolutdruck VOR dem DC-Blocker -- genau die
// Groesse, die der Stockwerk- und der Wassersaeulentest aus
// docs/05-kalibrierung.md brauchen.
void drawCalib(const vu::Report& r) {
    char buf[48];
    const float pa = r.rawPressurePa;

    tft.setTextColor(kDim, kBg);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("ABSOLUTDRUCK", 8, 6, 2);

    tft.setTextColor(kFg, kBg);
    snprintf(buf, sizeof(buf), "%8.2f hPa ", pa / 100.0f);
    tft.drawString(buf, 8, 26, 4);

    tft.setTextColor(kDim, kBg);
    snprintf(buf, sizeof(buf), "%9.1f Pa ", pa);
    tft.drawString(buf, 8, 56, 2);

    // Differenz zum gesetzten Bezugswert, zusaetzlich als Hoehendifferenz.
    // 12 Pa je Meter, das macht den Stockwerktest ohne Rechnen ablesbar.
    tft.setTextColor(kDim, kBg);
    tft.drawString("DIFFERENZ ZUR REFERENZ", 8, 84, 2);
    tft.setTextColor(kAccent, kBg);
    if (g_refSet) {
        const float d = pa - g_refPa;
        snprintf(buf, sizeof(buf), "%+8.1f Pa ", d);
        tft.drawString(buf, 8, 104, 4);
        snprintf(buf, sizeof(buf), "entspricht %+5.1f m Hoehe ", -d / kPaPerMeter);
        tft.drawString(buf, 8, 134, 2);
    } else {
        tft.drawString("-- keine gesetzt --      ", 8, 104, 4);
        tft.drawString("                              ", 8, 134, 2);
    }

    tft.setTextColor(kDim, kBg);
    snprintf(buf, sizeof(buf), "%.1f C   %.1f Hz   0x%02X   OVR %lu  ",
             static_cast<double>(g_tempC), g_fs, g_sensor.address(),
             static_cast<unsigned long>(g_overruns));
    tft.drawString(buf, 8, 164, 2);

    tft.setTextColor(r.pressurePlausible ? kDim : kWarn, kBg);
    tft.drawString(r.pressurePlausible ? "Taste: kurz = Schirm, lang = Referenz"
                                       : "Rohdruck unplausibel!             ",
                   8, kStatusY, 2);
}

// ---------------------------------------------------------------- Tasks

// Hohe Prioritaet, eigener Kern. Beim echten Sensor fasst der FIFO nur
// 32 Werte, das sind 51 ms bei 622 Hz -- laenger darf dieser Task nicht
// blockiert werden, sonst gehen Messwerte verloren.
void sensorTask(void*) {
    const TickType_t period = pdMS_TO_TICKS(cfg::kFifoReadIntervalMs);
    TickType_t last = xTaskGetTickCount();
    uint32_t prevUs = micros();

    for (;;) {
        if (!g_demoMode) {
            float samples[bmp581::kFifoMaxFrames];
            const int n = g_sensor.readFifo(samples, bmp581::kFifoMaxFrames);
            if (n > 0) {
                for (int i = 0; i < n; ++i) g_meter.process(samples[i]);
                g_samples += static_cast<uint32_t>(n);
                // Ein voller FIFO heisst, dass zwischen zwei Lesevorgaengen
                // Messwerte verloren gingen. Still weiterzurechnen waere
                // falsch -- die Anzeige weist es aus.
                if (g_sensor.overrun()) ++g_overruns;
            }
            // Temperatur selten mitlesen. Sie aendert sich langsam und der
            // Anzeige-Task darf den Bus nicht selbst benutzen.
            static uint32_t lastTemp = 0;
            if (millis() - lastTemp > 1000) {
                lastTemp = millis();
                float t = 0.0f;
                if (g_sensor.readTemperature(&t)) g_tempC = t;
            }
        } else {
            // Demobetrieb: so viele Werte erzeugen, wie in der verstrichenen
            // Zeit angefallen waeren. An der echten Uhr ausgerichtet, damit
            // die angezeigte Frequenz stimmt.
            const uint32_t now = micros();
            const uint32_t dtUs = now - prevUs;
            prevUs = now;
            int n = static_cast<int>(dtUs * 1e-6f * g_fs + 0.5f);
            if (n > 256) n = 256;  // nach einer Blockade nicht aufholen wollen
            const float dt = 1.0f / g_fs;
            for (int i = 0; i < n; ++i) g_meter.process(demoSample(dt));
            g_samples += n;
        }
        vTaskDelayUntil(&last, period);
    }
}

// Taster entprellen und zwischen kurz und lang unterscheiden.
// kurz -> Schirm wechseln, lang -> Haltewerte bzw. Referenz zuruecksetzen.
void handleButton(const vu::Report& r) {
    static bool wasDown = false;
    static uint32_t downAt = 0;

    const bool down = digitalRead(cfg::kButtonPin) == LOW;  // aktiv low
    const uint32_t now = millis();

    if (down && !wasDown) {
        downAt = now;
    } else if (!down && wasDown) {
        const uint32_t held = now - downAt;
        if (held < 40) {
            // Prellen, ignorieren
        } else if (held >= static_cast<uint32_t>(cfg::kLongPressMs)) {
            if (g_screen == Screen::Calib) {
                g_refPa = r.rawPressurePa;
                g_refSet = true;
                Serial.printf("Referenz gesetzt: %.1f Pa\n", g_refPa);
            } else {
                g_meter.resetHold();
                Serial.println("Haltewerte zurueckgesetzt");
            }
        } else {
            g_screen = (g_screen == Screen::Live) ? Screen::Calib : Screen::Live;
            tft.fillScreen(kBg);
            if (g_screen == Screen::Live) drawStatic();
        }
    }
    wasDown = down;
}

void uiTask(void*) {
    const TickType_t period = pdMS_TO_TICKS(1000 / cfg::kDisplayFps);
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        const vu::Report r = g_meter.report();
        handleButton(r);
        if (g_screen == Screen::Live) {
            drawReadings(r);
            drawSpectrum(g_meter);
        } else {
            drawCalib(r);
        }
        vTaskDelayUntil(&last, period);
    }
}

void splash(const char* line1, const char* line2, uint16_t color) {
    tft.fillScreen(kBg);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(color, kBg);
    tft.drawString(line1, 160, 100, 4);
    if (line2) {
        tft.setTextColor(kDim, kBg);
        tft.drawString(line2, 160, 140, 2);
    }
}

}  // namespace

void setup() {
    Serial.begin(115200);
    pinMode(cfg::kButtonPin, INPUT_PULLUP);

    tft.init();
    tft.setRotation(1);  // Querformat, 320 x 240

    // I2C vor dem Einschalten der Beleuchtung starten. Sollte je wieder ein
    // Pin doppelt belegt werden, faellt es dann wenigstens nicht erst nach
    // dem Einschalten auf -- die static_asserts in config.h verhindern es
    // ohnehin schon zur Uebersetzungszeit.
    Wire.begin(cfg::kI2cSda, cfg::kI2cScl, cfg::kI2cHz);
    setBacklight(cfg::kBacklightDefault);

    splash("Bass-SPL-Meter", "Sensor wird gesucht", TFT_WHITE);

    g_demoMode = !g_sensor.begin(Wire);

    if (!g_demoMode) {
        g_sensorAddr = g_sensor.address();
        Serial.printf("BMP581 auf 0x%02X, Abtastrate wird gemessen\n",
                      g_sensorAddr);
        char line[48];
        snprintf(line, sizeof(line), "%.0f s messen", cfg::kFsCalibSeconds);
        splash("BMP581 aktiv", line, TFT_GREEN);

        g_fs = measureSampleRate(cfg::kFsCalibSeconds);
        Serial.printf("gemessene Abtastrate: %.2f Hz\n", g_fs);

        snprintf(line, sizeof(line), "%.1f Hz gemessen", g_fs);
        splash("Abtastrate", line, TFT_GREEN);
    } else {
        Serial.println("Kein Sensor gefunden, Demobetrieb");
        splash("Kein Sensor", "Demobetrieb mit Testsignal", TFT_ORANGE);
        g_fs = cfg::kFsNominal;
    }
    delay(1500);

    if (!g_meter.init(g_fs, vu::Band::k10to100, cfg::kFftLive)) {
        // Kein stiller Rueckfall auf ein engeres Band: dann staende eine Zahl
        // auf dem Display, die etwas anderes misst als beschriftet. Lieber
        // deutlich sagen, was gemessen wurde und was noetig waere.
        const float needed = vu::bandLimits(vu::Band::k10to100).hi /
                             vu::kMaxBandFraction;
        char line[64];
        snprintf(line, sizeof(line), "%.0f Hz gemessen, %.0f Hz noetig", g_fs,
                 needed);
        splash("Abtastrate zu niedrig", line, TFT_RED);
        Serial.printf("Meter::init fehlgeschlagen: fs=%.2f Hz, benoetigt "
                      "mindestens %.2f Hz fuer das Band 10-100 Hz\n",
                      g_fs, needed);
        g_fsError = true;
        return;
    }

    drawStatic();
    xTaskCreatePinnedToCore(sensorTask, "sensor", 4096, nullptr, 10, nullptr, 0);
    xTaskCreatePinnedToCore(uiTask, "ui", 8192, nullptr, 3, nullptr, 1);
}

void loop() {
    // Alle 5 s ein Lebenszeichen auf die serielle Schnittstelle. Nuetzlich,
    // wenn das Display noch nicht richtig konfiguriert ist.
    static uint32_t last = 0;
    if (millis() - last > 5000) {
        last = millis();
        if (g_fsError) {
            Serial.printf("Abtastrate zu niedrig: %.2f Hz\n", g_fs);
        } else {
            const vu::Report r = g_meter.report();
            Serial.printf("%s  %6.1f dB  f0 %6.2f Hz  peak %6.1f  roh %.0f Pa  "
                          "n=%lu  ovr=%lu\n",
                          g_demoMode ? "DEMO" : "BMP", r.splSlow, r.f0,
                          r.splPeak, r.rawPressurePa,
                          static_cast<unsigned long>(g_samples),
                          static_cast<unsigned long>(g_overruns));
        }
    }
    vTaskDelay(pdMS_TO_TICKS(200));
}
