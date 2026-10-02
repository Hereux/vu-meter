// I2C-Anbindung des BMP581.
//
// Nur die Busanbindung. Registerwerte und Dekodierung stehen in
// bmp581_regs.h und sind im PC-Test abgedeckt; dieser Teil laesst sich nur
// auf dem Geraet pruefen.
#pragma once

#include <Wire.h>
#include <cstddef>
#include <cstdint>

#include "bmp581_regs.h"

class Bmp581 {
 public:
    // Sucht den Sensor auf beiden moeglichen Adressen und konfiguriert ihn
    // fuer die Messung: Dauerbetrieb, kein Oversampling, IIR-Filter umgangen,
    // FIFO nur mit Druckwerten.
    bool begin(TwoWire& wire);

    // Holt bis zu maxN Druckwerte in Pa aus dem FIFO.
    // Rueckgabe: Anzahl gelieferter Werte, oder -1 bei einem Busfehler.
    int readFifo(float* out, int maxN);

    // Momentanwert ohne FIFO, fuer Plausibilitaetspruefungen.
    bool readPressure(float* pa);
    bool readTemperature(float* degC);

    // Beim letzten Lesen war der FIFO voll -- dann sind mit hoher
    // Wahrscheinlichkeit Messwerte verloren gegangen.
    bool overrun() const { return overrun_; }
    uint8_t address() const { return addr_; }
    bool present() const { return addr_ != 0; }

 private:
    bool readRegs(uint8_t reg, uint8_t* buf, size_t len);
    bool writeReg(uint8_t reg, uint8_t value);
    bool configure();

    TwoWire* wire_ = nullptr;
    uint8_t addr_ = 0;
    bool overrun_ = false;
};
