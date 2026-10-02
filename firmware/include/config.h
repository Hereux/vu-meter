// Zentrale Konfiguration: Pins, Vorgaben, Grenzwerte.
#pragma once

namespace cfg {

// ---- Pins (ESP32-2432S028R) ----------------------------------------------
// Vor dem Loeten gegen die eigene Boardrevision pruefen -- von den CYD-Boards
// gibt es mehrere Varianten. Frei sind ueblicherweise GPIO 21, 22 und 35.
// GPIO 21 ist auf dem CYD die Hintergrundbeleuchtung und darf hier NICHT
// stehen, auch wenn 21/22 die Arduino-Standardpins fuer I2C sind: Wire.begin()
// konfiguriert den Pin um und schaltet damit das Licht aus. Frei und auf den
// Steckern herausgefuehrt sind 22, 27 und 35 (nur Eingang).
constexpr int kI2cSda = 27;
constexpr int kI2cScl = 22;
constexpr int kI2cHz = 400000;
constexpr int kSensorIntPin = 35;  // optional, nur Eingang
constexpr int kButtonPin = 0;      // Boot-Taster, alternativ Touch

// ---- Hintergrundbeleuchtung ----
// Aktiv HIGH. Wird per PWM angesteuert, damit sich die Helligkeit regeln
// laesst -- im dunklen Fahrzeug blendet volle Helligkeit, tagsueber reicht sie
// gerade so.
constexpr int kBacklightPin = 21;
constexpr int kBacklightPwmCh = 7;      // LEDC-Kanal
constexpr int kBacklightPwmHz = 5000;
constexpr int kBacklightBits = 8;       // 0 .. 255
constexpr int kBacklightDefault = 80;   // Prozent

// Sicherung gegen genau den Fehler, der das Licht einmal ausgeschaltet hat:
// liegt ein I2C-Pin auf dem Backlight-Pin, schaltet Wire.begin() ihn um.
static_assert(kI2cSda != kBacklightPin,
              "I2C-SDA liegt auf dem Backlight-Pin: Wire.begin() wuerde die "
              "Hintergrundbeleuchtung ausschalten");
static_assert(kI2cScl != kBacklightPin,
              "I2C-SCL liegt auf dem Backlight-Pin: Wire.begin() wuerde die "
              "Hintergrundbeleuchtung ausschalten");

// Adressen, Register und Kennungen des BMP581 stehen in src/bmp581_regs.h,
// belegt aus der offiziellen Bosch-API. Hier doppelt gefuehrt zu werden waere
// eine Einladung zum Auseinanderlaufen.

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
// FIFO fasst 32 Werte. Bei 622 Hz sind das 51 ms -- bei 40 ms Leseabstand
// blieben nur 11 ms Reserve fuer Jitter. 20 ms lassen rund 12 Rahmen je
// Lesevorgang und damit reichlich Luft.
constexpr int kFifoReadIntervalMs = 20;
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
