// Registerdefinitionen und Dekodierung des BMP581.
//
// Bewusst frei von Arduino und I2C: diese Datei enthaelt genau die Teile, in
// denen sich Fehler verstecken -- Bitpackung der Konfigurationsregister und
// Umrechnung der Rohbytes. Sie werden im PC-Test geprueft. Die eigentliche
// Busanbindung steht in bmp581.h/.cpp und laesst sich nur auf dem Geraet
// pruefen.
//
// Quelle der Werte: offizielle Bosch-API, bmp5_defs.h und bmp5.c aus
// https://github.com/boschsensortec/BMP5_SensorAPI -- nicht aus dem
// Gedaechtnis rekonstruiert.
#pragma once

#include <cstdint>

namespace bmp581 {

// ---- Register ----
constexpr uint8_t kRegChipId      = 0x01;
constexpr uint8_t kRegChipStatus  = 0x11;
constexpr uint8_t kRegDriveConfig = 0x13;
constexpr uint8_t kRegIntConfig   = 0x14;
constexpr uint8_t kRegIntSource   = 0x15;
constexpr uint8_t kRegFifoConfig  = 0x16;
constexpr uint8_t kRegFifoCount   = 0x17;
constexpr uint8_t kRegFifoSel     = 0x18;
constexpr uint8_t kRegTempData    = 0x1D;  // XLSB, LSB, MSB
constexpr uint8_t kRegPressData   = 0x20;  // XLSB, LSB, MSB
constexpr uint8_t kRegIntStatus   = 0x27;
constexpr uint8_t kRegStatus      = 0x28;
constexpr uint8_t kRegFifoData    = 0x29;
constexpr uint8_t kRegDspConfig   = 0x30;
constexpr uint8_t kRegDspIir      = 0x31;
constexpr uint8_t kRegOsrConfig   = 0x36;
constexpr uint8_t kRegOdrConfig   = 0x37;
constexpr uint8_t kRegOsrEff      = 0x38;
constexpr uint8_t kRegCmd         = 0x7E;

// ---- Kennungen ----
// Es gibt zwei gueltige Chip-IDs. Nur 0x50 zu akzeptieren wuerde Sensoren der
// zweiten Charge faelschlich als "nicht vorhanden" melden.
constexpr uint8_t kChipIdPrim = 0x50;
constexpr uint8_t kChipIdSec  = 0x51;
constexpr uint8_t kAddrPrim = 0x47;  // SDO high
constexpr uint8_t kAddrSec  = 0x46;  // SDO low

// ---- Betriebsarten (Bits 1:0 von ODR_CONFIG) ----
constexpr uint8_t kModeStandby    = 0;
constexpr uint8_t kModeNormal     = 1;
constexpr uint8_t kModeForced     = 2;
constexpr uint8_t kModeContinuous = 3;

// ---- Oversampling ----
constexpr uint8_t kOsr1x = 0;
constexpr uint8_t kOsr2x = 1;
constexpr uint8_t kOsr4x = 2;

// ---- IIR ----
constexpr uint8_t kIirBypass = 0x00;

// ---- FIFO ----
constexpr uint8_t kFifoDisabled    = 0;
constexpr uint8_t kFifoTempOnly    = 1;
constexpr uint8_t kFifoPressOnly   = 2;
constexpr uint8_t kFifoPressTemp   = 3;
constexpr uint8_t kFifoDecNone     = 0;
constexpr uint8_t kFifoCountMask   = 0x3F;
constexpr uint8_t kFifoEmptyByte   = 0x7F;
constexpr int kFifoMaxFrames       = 32;
constexpr int kFramePressBytes     = 3;
constexpr int kFramePressTempBytes = 6;

// ---- Wartezeiten aus dem Datenblatt ----
constexpr uint32_t kDelayStandbyUs   = 2500;
constexpr uint32_t kDelaySoftResetUs = 2000;
constexpr uint8_t  kCmdSoftReset     = 0xB6;

// ---- Umrechnung ----
// Druck: 24 Bit ohne Vorzeichen, LSB = 1/64 Pa.
// Temperatur: 24 Bit mit Vorzeichen, LSB = 1/65536 Grad C.
constexpr float kPressureLsbPa = 1.0f / 64.0f;
constexpr float kTempLsbC      = 1.0f / 65536.0f;

// ---------------------------------------------------------- Bitpackung

// OSR_CONFIG: Bit 6 press_en, Bits 5:3 osr_p, Bits 2:0 osr_t
constexpr uint8_t osrConfig(uint8_t osrPress, uint8_t osrTemp, bool pressEnable) {
    return static_cast<uint8_t>((pressEnable ? 0x40 : 0x00) |
                                ((osrPress & 0x07) << 3) | (osrTemp & 0x07));
}

// ODR_CONFIG: Bit 7 deep_dis, Bits 6:2 odr, Bits 1:0 pwr_mode
//
// deep_dis = 1 schaltet den Tiefschlaf ab. Ohne das faellt der Sensor bei
// niedriger Datenrate selbsttaetig hinein und liefert nichts mehr.
constexpr uint8_t odrConfig(uint8_t odr, uint8_t mode, bool deepDisable) {
    return static_cast<uint8_t>((deepDisable ? 0x80 : 0x00) |
                                ((odr & 0x1F) << 2) | (mode & 0x03));
}

// FIFO_SEL: Bits 4:2 dec_sel, Bits 1:0 frame_sel
constexpr uint8_t fifoSel(uint8_t frameSel, uint8_t decSel) {
    return static_cast<uint8_t>(((decSel & 0x07) << 2) | (frameSel & 0x03));
}

constexpr bool isChipIdValid(uint8_t id) {
    return id == kChipIdPrim || id == kChipIdSec;
}

constexpr int fifoFrameCount(uint8_t fifoCountReg) {
    return fifoCountReg & kFifoCountMask;
}

// -------------------------------------------------------- Dekodierung

// Druck aus einem Drei-Byte-Rahmen (XLSB, LSB, MSB) in Pa.
inline float pressureFromBytes(const uint8_t* b) {
    const uint32_t raw = (static_cast<uint32_t>(b[2]) << 16) |
                         (static_cast<uint32_t>(b[1]) << 8) |
                         static_cast<uint32_t>(b[0]);
    return static_cast<float>(raw) * kPressureLsbPa;
}

// Temperatur aus einem Drei-Byte-Rahmen in Grad C. 24 Bit mit Vorzeichen,
// deshalb Vorzeichenerweiterung auf 32 Bit.
inline float temperatureFromBytes(const uint8_t* b) {
    uint32_t raw = (static_cast<uint32_t>(b[2]) << 16) |
                   (static_cast<uint32_t>(b[1]) << 8) |
                   static_cast<uint32_t>(b[0]);
    if (raw & 0x800000u) raw |= 0xFF000000u;  // Vorzeichen erweitern
    return static_cast<float>(static_cast<int32_t>(raw)) * kTempLsbC;
}

// Ein Rahmen, dessen Bytes alle 0x7F sind, ist leer. Der FIFO liefert das,
// wenn mehr gelesen wird, als er enthaelt.
inline bool frameEmpty(const uint8_t* b, int len) {
    for (int i = 0; i < len; ++i) {
        if (b[i] != kFifoEmptyByte) return false;
    }
    return true;
}

}  // namespace bmp581
