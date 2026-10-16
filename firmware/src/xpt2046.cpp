#include "xpt2046.h"

namespace touch {
namespace {

// Eigene SPI-Instanz. Das Display benutzt HSPI (Pins 12/13/14/15), der
// Touch bekommt VSPI mit umgelegten Pins. Der ESP32 kann jedes Peripherie-
// signal auf nahezu jeden Pin legen, deshalb ist das moeglich, obwohl die
// Standardpins von VSPI andere sind.
SPIClass g_spi(VSPI);
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

uint16_t pressure() {
    if (!g_ready) return 0;

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

    // Uebliche Naeherung fuer den Beruehrungsdruck. Unberuehrt geht z1 gegen
    // null und z2 gegen Vollausschlag, das Ergebnis also gegen null.
    const int32_t z = static_cast<int32_t>(z1) + 4095 - static_cast<int32_t>(z2);
    if (z < 0) return 0;
    if (z > 4095) return 4095;
    return static_cast<uint16_t>(z);
}

bool pressed(uint16_t threshold) { return pressure() > threshold; }

}  // namespace touch
