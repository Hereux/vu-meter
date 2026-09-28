// Zielsystem-Gerüst für den ESP32 (Phase 2 und 3).
//
// ACHTUNG: Dieser Teil ist ungetestet -- er lässt sich ohne die Hardware weder
// bauen noch prüfen. Die Messkette darunter (dsp/spectrum/meter) ist dagegen
// vollständig auf dem PC abgesichert: make -C firmware test.
//
// Offen, mit TODO markiert: der BMP581-Treiber (Phase 2) und die Anzeige
// (Phase 4). Die Struktur der Tasks und die Abtastraten-Kalibrierung stehen.

#include <Arduino.h>
#include <Wire.h>

#include "../include/config.h"
#include "meter.h"

namespace {

vu::Meter g_meter;
float g_fsMeasured = cfg::kFsNominal;

// Der Sensor taktet mit einem internen RC-Oszillator, die reale Abtastrate
// weicht bis zu +/-5 % vom Nennwert ab. Das ginge 1:1 in die Frequenzanzeige
// ein -- 5 % bei 40 Hz sind 2 Hz. Deshalb wird sie einmal beim Start gemessen
// und daraus werden Filter und FFT-Frequenzachse ausgelegt.
float calibrateSampleRate(float seconds) {
  const uint32_t t0 = micros();
  uint32_t counted = 0;
  while ((micros() - t0) < static_cast<uint32_t>(seconds * 1e6f)) {
    // TODO(Phase 2): FIFO-Füllstand lesen, Werte verwerfen, counted erhöhen.
    // Overrun-Flag prüfen und die Kalibrierung bei Overrun neu starten,
    // sonst zählt man zu wenige Samples und die Rate kommt zu niedrig heraus.
    delay(cfg::kFifoReadIntervalMs);
  }
  const uint32_t dtUs = micros() - t0;
  return counted > 0 ? (counted * 1e6f / static_cast<float>(dtUs))
                     : cfg::kFsNominal;
}

// Sensor-Task: höchste Priorität, eigener Kern. Der FIFO fasst nur 32 Werte,
// bei 622 Hz sind das 51 ms -- eine längere Blockade durch den
// Display-Refresh kostet Samples und damit die Messung.
void sensorTask(void*) {
  for (;;) {
    // TODO(Phase 2): FIFO-Burst lesen, Overrun-Flag auswerten,
    // jeden Wert durch g_meter.process(pa) schicken.
    vTaskDelay(pdMS_TO_TICKS(cfg::kFifoReadIntervalMs));
  }
}

// Anzeige-Task: niedrigere Priorität, anderer Kern.
void uiTask(void*) {
  for (;;) {
    const vu::Report r = g_meter.report();
    (void)r;  // TODO(Phase 4): Screens LIVE / PEAK / RTA zeichnen
    vTaskDelay(pdMS_TO_TICKS(1000 / cfg::kDisplayFps));
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Wire.begin(cfg::kI2cSda, cfg::kI2cScl, cfg::kI2cHz);

  // TODO(Phase 2): BMP581 initialisieren -- Chip-ID prüfen, Continuous Mode,
  // Oversampling 1x, IIR-Filter auf Bypass (er würde die Messung verfälschen),
  // FIFO einschalten.

  g_fsMeasured = calibrateSampleRate(cfg::kFsCalibSeconds);
  Serial.printf("gemessene Abtastrate: %.2f Hz\n", g_fsMeasured);

  if (!g_meter.init(g_fsMeasured, vu::Band::k10to100, cfg::kFftLive)) {
    Serial.println("FEHLER: Abtastrate zu niedrig für das gewählte Messband");
    // TODO(Phase 4): Fehlermeldung auf dem Display statt stillem Weiterlaufen
    return;
  }

  xTaskCreatePinnedToCore(sensorTask, "sensor", 4096, nullptr, 10, nullptr, 0);
  xTaskCreatePinnedToCore(uiTask, "ui", 8192, nullptr, 3, nullptr, 1);
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }
