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

// ---- Taster ----
// GPIO 35 statt 0. GPIO 0 ist der Boot-Taster des Boards und ein
// Strapping-Pin: liegt er beim Reset auf Masse, startet der ESP32 in den
// Bootloader statt in die Anwendung. Bei einem seitlichen Taster am Geraet
// heisst das, dass jedes Einschalten mit gedruecktem Taster wie ein Defekt
// aussieht -- im Fahrzeug mit Zuendungsplus passiert das frueher oder
// spaeter. Ausserdem ist GPIO 0 auf dem CYD gar nicht auf einen Stecker
// herausgefuehrt, ein externer Taster muesste an die Pads geloetet werden.
//
// Damit bleibt GPIO 35: der einzige noch freie herausgefuehrte Pin. Der
// bisher dort vorgesehene Data-Ready-Interrupt des BMP581 entfaellt dafuer
// ersatzlos -- gebraucht wurde er nie, die FIFO wird alle 20 ms gepollt
// (kFifoReadIntervalMs), und bei gemessenen 494 Hz Abtastrate laeuft sie
// dabei nie ueber.
constexpr int kButtonPin = 35;

// GPIO 34..39 sind REINE EINGAENGE OHNE interne Pull-Widerstaende.
// pinMode(pin, INPUT_PULLUP) wird dort stillschweigend ignoriert; der Pin
// bleibt offen und liefert Zufallswerte. Ein Taster auf diesen Pins braucht
// deshalb zwingend einen externen Pull-Up, 10 kOhm gegen 3V3.
constexpr bool isInputOnly(int pin) { return pin >= 34 && pin <= 39; }

// Strapping-Pins legen beim Reset den Bootmodus fest. Kein Taster darauf.
constexpr bool isStrapping(int pin) {
    return pin == 0 || pin == 2 || pin == 5 || pin == 12 || pin == 15;
}

// Wird in main.cpp fuer pinMode ausgewertet und steuert die Startpruefung.
constexpr bool kButtonExternalPullup = isInputOnly(kButtonPin);

static_assert(!isStrapping(kButtonPin),
              "Taster auf einem Strapping-Pin: wer beim Einschalten drueckt, "
              "landet im Bootloader statt in der Anwendung");
static_assert(kButtonPin != kI2cSda && kButtonPin != kI2cScl,
              "Taster liegt auf einem I2C-Pin");

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
static_assert(kButtonPin != kBacklightPin,
              "Taster liegt auf dem Backlight-Pin");

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

// ---- Messprofile ----
// Jedes Profil buendelt Messband und Spektrumsskala. Umschaltbar per Taste.
//
// Zu den Skalengrenzen: der angezeigte Gesamtpegel verteilt sich auf alle
// Bins des Bandes. Bei 86 dB Gesamtpegel und rund 93 Bins traegt ein
// einzelner Balken nur etwa 67 dB. Eine Skala, die erst bei 85 dB beginnt,
// bliebe im Zimmer deshalb leer -- die Untergrenze muss deutlich unter dem
// erwarteten Gesamtpegel liegen.
constexpr float kProfileRoomSpecMin = 50.0f;
constexpr float kProfileRoomSpecMax = 110.0f;
constexpr float kProfileCarSpecMin = 100.0f;
constexpr float kProfileCarSpecMax = 170.0f;

// ---- Anzeige --------------------------------------------------------------
constexpr int kDisplayFps = 5;
constexpr int kLongPressMs = 600;  // Peak-Reset

}  // namespace cfg
