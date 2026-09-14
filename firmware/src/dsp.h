// Zeitbereichspfad der Messkette: DC-Blocker, Bandfilter, Pegeldetektoren.
//
// Portierung von tools/dsp_model.py. Bewusst frei von Arduino- und
// ESP-IDF-Abhaengigkeiten, damit derselbe Code im PC-Test laeuft, der ihn gegen
// die Referenzwerte aus dem Python-Modell prueft.
#pragma once

namespace vu {

constexpr float kPRef = 20e-6f;  // Pa, Bezugsschalldruck fuer 0 dB SPL

// Zeitkonstanten der Pegeldetektoren, wie bei Schallpegelmessern
constexpr float kTauFast = 0.125f;
constexpr float kTauSlow = 1.0f;

// Sperrzeit fuer die Haltewerte nach Start, Reset oder Bandwechsel. Das
// Bandfilter ueberschwingt beim Einschwingen um gut 1 dB; ohne Sperre landet
// dieser Ueberschwinger als vermeintlicher Spitzenpegel im Peak-Hold.
constexpr float kHoldGuardS = 0.15f;

constexpr float kDcBlockerFc = 0.5f;  // Hz

float splFromPa(float pRms);
float paFromSpl(float splDb);

// Korrektur des Sensor-Tiefpasses: der ADC mittelt ueber ein Abtastintervall.
float sincCorrectionDb(float f, float fs);

// Guetefaktoren der Butterworth-Sektionen. order muss gerade sein, q fasst
// order/2 Werte.
void butterworthQ(int order, float* q);

class Biquad {
 public:
  static Biquad lowpass(float fc, float fs, float q);
  static Biquad highpass(float fc, float fs, float q);

  float process(float x) {
    const float y = b0 * x + z1_;
    z1_ = b1 * x - a1 * y + z2_;
    z2_ = b2 * x - a2 * y;
    return y;
  }
  void reset() { z1_ = z2_ = 0.0f; }
  // Betrag des Frequenzgangs bei f in dB.
  float magnitudeDb(float f, float fs) const;

  float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;

 private:
  float z1_ = 0.0f, z2_ = 0.0f;
};

// Hochpass 1. Ordnung. Entfernt den statischen Luftdruck und langsame Drift.
class DcBlocker {
 public:
  void design(float fc, float fs);
  void reset() { x1_ = y1_ = 0.0f; primed_ = false; }
  float process(float x);

 private:
  float a_ = 1.0f, x1_ = 0.0f, y1_ = 0.0f;
  bool primed_ = false;
};

// Messbandfilter: Butterworth-Hochpass + -Tiefpass, je 4. Ordnung.
// -3 dB an den Bandgrenzen, 24 dB/Oktave Flankensteilheit.
class BandFilter {
 public:
  static constexpr int kSections = 4;

  // Koeffizienten werden aus der tatsaechlich gemessenen Abtastrate berechnet.
  // Feste Tabellen waeren um den Fehler des Sensor-RC-Oszillators daneben
  // (bis +/-5 %), und zwar sowohl in den Bandgrenzen als auch in der
  // Frequenzachse der FFT.
  void design(float fLo, float fHi, float fs);
  void reset();
  float process(float x);
  float responseDb(float f) const;

  Biquad sections[kSections];
  float fLo = 0.0f, fHi = 0.0f, fs = 0.0f;
};

// Exponentieller RMS-Detektor plus True-Peak- und Max-RMS-Halteglied.
class LevelDetector {
 public:
  void init(float fs, float tau);
  // hold=false friert die Haltewerte ein (Einschwingsperre).
  float process(float x, bool hold);
  void resetHold() { peak_ = 0.0f; maxRms_ = 0.0f; }

  float rms() const;
  float peak() const { return peak_; }
  float splRms() const { return splFromPa(rms()); }
  float splMaxRms() const { return splFromPa(maxRms_); }
  // True-Peak als Pegel. Bezug bleibt 20 uPa, also kein /sqrt(2).
  float splPeak() const { return splFromPa(peak_); }

 private:
  float alpha_ = 0.0f, ms_ = 0.0f, peak_ = 0.0f, maxRms_ = 0.0f;
};

}  // namespace vu
