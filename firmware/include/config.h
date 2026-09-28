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

// ---- Sensor: Erkennung ohne Herstellerbibliothek ----
// Der BMP581 meldet sich auf 0x47 (SDO high) oder 0x46 (SDO low) und liefert
// in Register 0x01 die Chip-ID 0x50. Das reicht, um festzustellen, ob ein
// Sensor angeschlossen ist -- ohne eine Bibliothek einzubinden, die erst in
// Phase 2 gebraucht wird.
constexpr int kBmp581Addr[2] = {0x47, 0x46};
constexpr int kBmp581RegChipId = 0x01;
constexpr int kBmp581ChipId = 0x50;

// ---- Demobetrieb ohne Sensor ----
// Wird kein Sensor gefunden, speist das Geraet ein synthetisches Signal durch
// dieselbe Messkette und zeigt es an. Damit laesst sich das Display und die
// gesamte Rechenkette auf echter Hardware pruefen, bevor der Sensor da ist --
// und man sieht sofort, ob die Anzeige plausible Werte liefert.
constexpr float kDemoSpl = 150.0f;   // Pegel des Testsignals
constexpr float kDemoFreqLo = 20.0f; // Sweep von ...
constexpr float kDemoFreqHi = 80.0f; // ... bis
constexpr float kDemoSweepS = 12.0f; // Dauer eines Durchlaufs
constexpr float kDemoAmbientPa = 101325.0f;

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
