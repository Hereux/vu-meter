#include "bmp581.h"

#include <Arduino.h>

namespace {
// Der FIFO fasst 32 Rahmen zu je drei Bytes. Das passt in einen Zug in den
// I2C-Puffer des ESP32 (128 Byte).
constexpr int kBurstBytes = bmp581::kFifoMaxFrames * bmp581::kFramePressBytes;
}  // namespace

bool Bmp581::readRegs(uint8_t reg, uint8_t* buf, size_t len) {
    wire_->beginTransmission(addr_);
    wire_->write(reg);
    if (wire_->endTransmission(false) != 0) return false;  // Repeated Start
    if (wire_->requestFrom(static_cast<int>(addr_), static_cast<int>(len)) !=
        static_cast<int>(len)) {
        return false;
    }
    for (size_t i = 0; i < len; ++i) buf[i] = static_cast<uint8_t>(wire_->read());
    return true;
}

bool Bmp581::writeReg(uint8_t reg, uint8_t value) {
    wire_->beginTransmission(addr_);
    wire_->write(reg);
    wire_->write(value);
    return wire_->endTransmission() == 0;
}

bool Bmp581::begin(TwoWire& wire) {
    wire_ = &wire;
    addr_ = 0;

    const uint8_t candidates[2] = {bmp581::kAddrPrim, bmp581::kAddrSec};
    for (uint8_t a : candidates) {
        addr_ = a;
        uint8_t id = 0;
        if (readRegs(bmp581::kRegChipId, &id, 1) && bmp581::isChipIdValid(id)) {
            return configure();
        }
    }
    addr_ = 0;
    return false;
}

bool Bmp581::configure() {
    // Reihenfolge ist vorgeschrieben: der Sensor muss im Standby stehen,
    // bevor Konfigurationsregister beschrieben werden, und erst danach in den
    // Dauerbetrieb wechseln. Schreibt man mitten im Betrieb, uebernimmt er die
    // Werte nicht zuverlaessig.
    if (!writeReg(bmp581::kRegOdrConfig,
                  bmp581::odrConfig(0, bmp581::kModeStandby, true))) {
        return false;
    }
    delayMicroseconds(bmp581::kDelayStandbyUs);

    // IIR-Filter umgehen. Er wuerde als zusaetzlicher Tiefpass genau im
    // Messband wirken und den Frequenzgang verfaelschen -- die Filterung
    // macht die Firmware selbst, mit bekannten Koeffizienten.
    if (!writeReg(bmp581::kRegDspIir, bmp581::kIirBypass)) return false;

    // Kein Oversampling: jede Mittelung kostet Abtastrate, und die brauchen
    // wir fuer die Bandbreite. Das Rauschen liegt auch so weit unter den
    // Pegeln, um die es hier geht.
    if (!writeReg(bmp581::kRegOsrConfig,
                  bmp581::osrConfig(bmp581::kOsr1x, bmp581::kOsr1x, true))) {
        return false;
    }

    // FIFO nur mit Druckwerten: drei statt sechs Bytes je Rahmen, also die
    // doppelte Anzahl Messwerte je Auslesevorgang.
    if (!writeReg(bmp581::kRegFifoSel,
                  bmp581::fifoSel(bmp581::kFifoPressOnly, bmp581::kFifoDecNone))) {
        return false;
    }
    // Streaming: laeuft der FIFO ueber, werden die aeltesten Werte verworfen.
    // Fuer ein Messgeraet richtig herum -- der neueste Wert zaehlt.
    if (!writeReg(bmp581::kRegFifoConfig, 0x00)) return false;

    // Dauerbetrieb. Die Datenrate ergibt sich aus dem Oversampling, das
    // ODR-Feld wird dabei ignoriert; die tatsaechliche Rate misst die Firmware
    // beim Start.
    if (!writeReg(bmp581::kRegOdrConfig,
                  bmp581::odrConfig(0, bmp581::kModeContinuous, true))) {
        return false;
    }
    delayMicroseconds(bmp581::kDelayStandbyUs);
    overrun_ = false;
    return true;
}

int Bmp581::readFifo(float* out, int maxN) {
    if (addr_ == 0 || maxN <= 0) return 0;

    uint8_t countReg = 0;
    if (!readRegs(bmp581::kRegFifoCount, &countReg, 1)) return -1;

    int frames = bmp581::fifoFrameCount(countReg);
    if (frames <= 0) return 0;

    // Der Sensor meldet keinen eigenen Ueberlauf-Merker fuer den
    // Streaming-Betrieb. Ein voller FIFO heisst aber, dass zwischen zwei
    // Lesevorgaengen zu viel Zeit vergangen ist -- dann fehlen Messwerte.
    overrun_ = (frames >= bmp581::kFifoMaxFrames);

    if (frames > maxN) frames = maxN;
    const size_t bytes =
        static_cast<size_t>(frames) * bmp581::kFramePressBytes;

    uint8_t buf[kBurstBytes];
    if (!readRegs(bmp581::kRegFifoData, buf, bytes)) return -1;

    int n = 0;
    for (int i = 0; i < frames; ++i) {
        const uint8_t* frame = buf + i * bmp581::kFramePressBytes;
        // Der FIFO fuellt nicht vorhandene Rahmen mit 0x7F. Taucht so einer
        // auf, ist der Rest des Blocks ebenfalls leer.
        if (bmp581::frameEmpty(frame, bmp581::kFramePressBytes)) break;
        out[n++] = bmp581::pressureFromBytes(frame);
    }
    return n;
}

bool Bmp581::readPressure(float* pa) {
    uint8_t b[3];
    if (!readRegs(bmp581::kRegPressData, b, 3)) return false;
    *pa = bmp581::pressureFromBytes(b);
    return true;
}

bool Bmp581::readTemperature(float* degC) {
    uint8_t b[3];
    if (!readRegs(bmp581::kRegTempData, b, 3)) return false;
    *degC = bmp581::temperatureFromBytes(b);
    return true;
}
