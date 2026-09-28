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
#include "meter.h"

namespace {

TFT_eSPI tft;
vu::Meter g_meter;

float g_fs = cfg::kFsNominal;
int g_sensorAddr = -1;   // -1 = kein Sensor gefunden, Demobetrieb
uint32_t g_samples = 0;  // verarbeitete Messwerte, fuer die Statuszeile

// ------------------------------------------------------------------ Sensor

// Liest ein Register ueber I2C. Gibt false zurueck, wenn niemand antwortet.
bool readReg(int addr, uint8_t reg, uint8_t* value) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(addr, 1) != 1) return false;
    *value = Wire.read();
    return true;
}

// Sucht den BMP581 auf beiden moeglichen Adressen.
int findSensor() {
    for (int i = 0; i < 2; ++i) {
        uint8_t id = 0;
        if (readReg(cfg::kBmp581Addr[i], cfg::kBmp581RegChipId, &id) &&
            id == cfg::kBmp581ChipId) {
            return cfg::kBmp581Addr[i];
        }
    }
    return -1;
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

constexpr uint16_t kBg = TFT_BLACK;
constexpr uint16_t kFg = TFT_WHITE;
constexpr uint16_t kDim = 0x7BEF;  // mittleres Grau
constexpr uint16_t kAccent = TFT_CYAN;
constexpr uint16_t kWarn = TFT_ORANGE;

constexpr int kSpecY = 150;   // Oberkante des Spektrums
constexpr int kSpecH = 78;
constexpr int kSpecX = 8;
constexpr int kSpecW = 304;

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
    char buf[32];

    // Grosser Pegelwert
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(kFg, kBg);
    snprintf(buf, sizeof(buf), "%5.1f", r.splSlow);
    tft.drawString(buf, 228, 30, 7);

    // Dominante Frequenz
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(kAccent, kBg);
    snprintf(buf, sizeof(buf), "%6.2f Hz", r.f0);
    tft.drawString(buf, 8, 106, 4);

    // Spitzenwerte
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(kDim, kBg);
    snprintf(buf, sizeof(buf), "max %5.1f  peak %5.1f", r.splMaxRms, r.splPeak);
    tft.drawString(buf, 312, 112, 2);

    // Statuszeile
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(r.pressurePlausible ? kDim : kWarn, kBg);
    snprintf(buf, sizeof(buf), "%s Hz  %.0f/s  %s", r.band, g_fs,
             g_sensorAddr < 0 ? "DEMO" : "BMP581");
    tft.drawString(buf, 8, 224, 2);
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
        if (g_sensorAddr >= 0) {
            // TODO(Phase 2): FIFO-Burst lesen, Overrun-Flag auswerten, jeden
            // Wert durch g_meter.process(pa) schicken.
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

void uiTask(void*) {
    const TickType_t period = pdMS_TO_TICKS(1000 / cfg::kDisplayFps);
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        const vu::Report r = g_meter.report();
        drawReadings(r);
        drawSpectrum(g_meter);
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

    tft.init();
    tft.setRotation(1);  // Querformat, 320 x 240
    splash("Bass-SPL-Meter", "Sensor wird gesucht", TFT_WHITE);

    Wire.begin(cfg::kI2cSda, cfg::kI2cScl, cfg::kI2cHz);
    g_sensorAddr = findSensor();

    if (g_sensorAddr >= 0) {
        Serial.printf("BMP581 auf 0x%02X gefunden\n", g_sensorAddr);
        // TODO(Phase 2): Continuous Mode, Oversampling 1x, IIR auf Bypass,
        // FIFO einschalten, dann die Abtastrate ueber 30 s messen.
        splash("BMP581 gefunden", "Treiber folgt in Phase 2", TFT_GREEN);
    } else {
        Serial.println("Kein Sensor gefunden, Demobetrieb");
        splash("Kein Sensor", "Demobetrieb mit Testsignal", TFT_ORANGE);
    }
    delay(1500);

    if (!g_meter.init(g_fs, vu::Band::k10to100, cfg::kFftLive)) {
        splash("FEHLER", "Abtastrate zu niedrig fuer das Band", TFT_RED);
        Serial.println("Meter::init fehlgeschlagen");
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
        const vu::Report r = g_meter.report();
        Serial.printf("%s  %6.1f dB  f0 %6.2f Hz  peak %6.1f  roh %.0f Pa  n=%lu\n",
                      g_sensorAddr < 0 ? "DEMO" : "BMP", r.splSlow, r.f0,
                      r.splPeak, r.rawPressurePa,
                      static_cast<unsigned long>(g_samples));
    }
    vTaskDelay(pdMS_TO_TICKS(200));
}
