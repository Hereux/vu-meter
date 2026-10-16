// Minimaltreiber fuer den Touchcontroller XPT2046 des ESP32-2432S028R.
//
// Warum nicht ueber TFT_eSPI: der XPT2046 haengt auf dem CYD an einem
// ANDEREN SPI-Bus als das Display (siehe Kommentar zu TOUCH_CS in
// platformio.ini). Mit gesetztem TOUCH_CS wuerde TFT_eSPI ihn ueber den
// Display-Bus ansprechen, und der Touch bliebe tot.
//
// Warum keine fertige Bibliothek: gebraucht wird genau eine Information --
// "wird gerade beruehrt?". Dafuer reichen zwei SPI-Transfers. Eine
// Abhaengigkeit mehr waere hier Risiko ohne Gegenwert, zumal der Build
// sonst an einem geaenderten Registry-Namen scheitern kann.
//
// Bewusst OHNE Koordinaten und ohne Kalibrierung: die Bedienung nutzt den
// ganzen Schirm als eine Flaeche. Das war die Vorgabe -- einzelne
// Schaltflaechen muessten gross genug fuer Finger ohne Stift sein und
// wuerden den Platz wegnehmen, auf dem die Messwerte stehen.
#pragma once

// Arduino.h zuerst und ausdruecklich: SPI.h des ESP32-Cores zieht es NICHT
// mit herein, pinMode, digitalWrite, HIGH/LOW und MSBFIRST kaemen sonst
// nicht an.
#include <Arduino.h>
#include <SPI.h>
#include <stdint.h>

namespace touch {

// Die Beruehrung wird ueber den Widerstand zwischen den Panelebenen
// erkannt. Der Rohwert steigt mit dem Druck; unberuehrt liegt er nahe null.
// Schwelle grosszuegig, es geht nur um "Finger drauf oder nicht".
constexpr uint16_t kDefaultThreshold = 400;

// Pins des Touchcontrollers auf dem CYD. Eigener Bus, nicht die des
// Displays. 36 (IRQ) und 39 (MISO) sind reine Eingaenge -- hier kein
// Problem, beide werden nur gelesen.
constexpr int kPinSck = 25;
constexpr int kPinMiso = 39;
constexpr int kPinMosi = 32;
constexpr int kPinCs = 33;

// Der XPT2046 gibt erst unterhalb von etwa 2 MHz saubere Wandlungsergebnisse.
constexpr uint32_t kSpiHz = 2000000;

// Richtet den eigenen SPI-Bus ein. Einmal in setup() aufrufen, bevor
// pressure() oder pressed() benutzt werden.
void begin();

// Rohwert des Beruehrungsdrucks, 0 .. etwa 4095. Fuer Diagnose und zum
// Einstellen der Schwelle.
//
// Ueber z1_out und z2_out kommen auf Wunsch die beiden Rohkanaele DERSELBEN
// Messung mit heraus. Damit laesst sich unterscheiden:
//   z1 = 0, z2 = 0        keine Antwort, MISO liegt fest auf Masse
//   z1 = 4095, z2 = 4095  keine Antwort, MISO haengt offen
//   z1 klein, z2 gross    Panel antwortet, unberuehrt
//   z1 und z2 genaehert   Panel antwortet, beruehrt
//
// Bewusst als Ausgabeparameter und nicht ueber gespeicherte Werte: auf den
// Treiber greifen drei Tasks zu (Eingabe mit 50 Hz, Anzeige mit 5 Hz,
// serielles Lebenszeichen). Lagen die Rohwerte in Globals, konnte zwischen
// "Druck holen" und "Rohwerte holen" eine fremde Messung dazwischenfunken --
// die angezeigten Zahlen passten dann nicht zueinander.
uint16_t pressure(uint16_t* z1_out = nullptr, uint16_t* z2_out = nullptr);

// true, solange das Panel beruehrt wird.
bool pressed(uint16_t threshold = kDefaultThreshold);

}  // namespace touch
