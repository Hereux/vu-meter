#include "xpt2046.h"

namespace touch {
namespace {

// Eigene SPI-Instanz auf HSPI.
//
// HSPI und nicht VSPI, und das ist der Knackpunkt: TFT_eSPI benutzt auf dem
// ESP32 ohne USE_HSPI_PORT das globale SPI-Objekt -- und das ist VSPI. Dass
// die Displaypins 12/13/14/15 zufaellig die Standardpins von HSPI sind,
// aendert daran nichts; der ESP32 legt Peripheriesignale ueber die
// GPIO-Matrix auf beliebige Pins. Beide Treiber auf demselben Peripherie-
// geraet heisst: wer zuletzt begin() ruft, legt die Pins um, und der andere
// sendet ins Leere.
SPIClass g_spi(HSPI);
bool g_ready = false;

// Steuerbytes des XPT2046.
//   Bit 7    Startbit, immer 1
//   Bit 6..4 Kanal
//   Bit 3    0 = 12 Bit Aufloesung
//   Bit 2    0 = differentiell (temperaturstabiler als Single-Ended)
//   Bit 1..0 Power-Down-Modus
constexpr uint8_t kCmdZ1 = 0xB1;
constexpr uint8_t kCmdZ2 = 0xC1;

// Eine Wandlung anstossen und das Ergebnis holen.
//
// Der Ablauf ist vom Bauteil vorgegeben: das Steuerbyte schieben, dann zwei
// Bytes nachtakten, waehrend der Wandler das Ergebnis herausschiebt. Es
// kommt linksbuendig in 16 Bit, die unteren drei Bits sind Fuellbits.
uint16_t readChannel(uint8_t cmd) {
    g_spi.transfer(cmd);
    const uint8_t hi = g_spi.transfer(0x00);
    const uint8_t lo = g_spi.transfer(0x00);
    return static_cast<uint16_t>((hi << 8 | lo) >> 3) & 0x0FFF;
}

}  // namespace

void begin() {
    pinMode(kPinCs, OUTPUT);
    digitalWrite(kPinCs, HIGH);
    g_spi.begin(kPinSck, kPinMiso, kPinMosi, kPinCs);
    g_ready = true;
}

uint16_t pressure(uint16_t* z1_out, uint16_t* z2_out) {
    if (!g_ready) {
        if (z1_out) *z1_out = 0;
        if (z2_out) *z2_out = 0;
        return 0;
    }

    g_spi.beginTransaction(SPISettings(kSpiHz, MSBFIRST, SPI_MODE0));
    digitalWrite(kPinCs, LOW);

    // Die erste Wandlung nach dem Aktivieren von CS ist unzuverlaessig --
    // der Wandler braucht eine Erfassungsphase. Also verwerfen und erst den
    // zweiten Wert nehmen.
    readChannel(kCmdZ1);
    const uint16_t z1 = readChannel(kCmdZ1);
    const uint16_t z2 = readChannel(kCmdZ2);

    digitalWrite(kPinCs, HIGH);
    g_spi.endTransaction();

    if (z1_out) *z1_out = z1;
    if (z2_out) *z2_out = z2;

    // Plausibilitaet VOR der Rechnung. Bei einem echten Panel ist z1 immer
    // kleiner als z2: unberuehrt geht z1 gegen null und z2 gegen
    // Vollausschlag, unter dem Finger naehern sie sich an, ohne die
    // Reihenfolge zu tauschen.
    //
    // Antwortet der Bus gar nicht, liest man entweder lauter Nullen oder
    // lauter Einsen -- beides ergibt in der Formel unten 4095 und damit
    // "dauerhaft beruehrt". Das ist die unangenehmste Fehlerart: die
    // Bedienung reagiert dann auf gar nichts mehr, weil ohne Loslassen auch
    // kein Tastendruck zustande kommt. Genau so ist es passiert. Die
    // Abfrage macht daraus "nicht beruehrt".
    if (z1 >= z2) return 0;

    // Uebliche Naeherung fuer den Beruehrungsdruck.
    const int32_t z = static_cast<int32_t>(z1) + 4095 - static_cast<int32_t>(z2);
    if (z < 0) return 0;
    if (z > 4095) return 4095;
    return static_cast<uint16_t>(z);
}

bool pressed(uint16_t threshold) { return pressure() > threshold; }

}  // namespace touch
