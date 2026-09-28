// Zentrale Konfiguration: Pins, Vorgaben, Grenzwerte.
#pragma once

namespace cfg {

// ---- Pins (ESP32-2432S028R) ----------------------------------------------
// Vor dem Loeten gegen die eigene Boardrevision pruefen -- von den CYD-Boards
// gibt es mehrere Varianten. Frei sind ueblicherweise GPIO 21, 22 und 35.
constexpr int kI2cSda = 21;
constexpr int kI2cScl = 22;
constexpr int kI2cHz = 400000;
constexpr int kSensorIntPin = 35;  // optional, nur Eingang
constexpr int kButtonPin = 0;      // Boot-Taster, alternativ Touch

// ---- Sensor ---------------------------------------------------------------
constexpr float kFsNominal = 622.0f;   // Continuous Mode, OSR 1x
constexpr float kFsCalibSeconds = 30.0f;  // Dauer der Abtastraten-Kalibrierung
constexpr int kFifoReadIntervalMs = 40;   // FIFO fasst 32 Werte = 51 ms
// Plausibilitaetsgrenzen fuer den Rohdruck. Ausserhalb -> "OVER" anzeigen.
constexpr float kPressureMinPa = 35000.0f;
constexpr float kPressureMaxPa = 120000.0f;

// ---- Messung --------------------------------------------------------------
constexpr int kFftLive = 512;   // 1,21 Hz Bins bei 622 Hz, Update 4,9/s
constexpr int kFftRta = 1024;   // 0,61 Hz Bins, Update 2,4/s
constexpr float kClipWarnSpl = 175.0f;

// ---- Anzeige --------------------------------------------------------------
constexpr int kDisplayFps = 5;
constexpr int kLongPressMs = 600;  // Peak-Reset

}  // namespace cfg
