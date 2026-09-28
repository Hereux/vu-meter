// Gesamtkette: verbindet Zeitbereichs- und Spektralpfad zu einem Messgeraet.
#pragma once

#include "dsp.h"
#include "spectrum.h"

namespace vu {

enum class Band { k10to100 = 0, k5to150 = 1 };
constexpr int kBandCount = 2;

// Groesster zulaessiger Anteil der Abtastrate fuer die obere Bandgrenze.
// Genau auf Nyquist entartet der Tiefpass (w0 = pi -> alpha = 0, das Filter
// verliert seine Wirkung), und schon knapp darunter wird die Flanke unbrauchbar
// verzerrt. 0,4 laesst mindestens 2,5 Abtastwerte pro Periode der hoechsten
// Messfrequenz. Praktische Folge: ein BMP390 mit 200 Hz schafft hoechstens
// 80 Hz obere Bandgrenze, also weder 10-100 noch 5-150.
constexpr float kMaxBandFraction = 0.4f;

struct BandLimits {
  float lo, hi;
  const char* name;
};

// Die Grenzen sind die -3-dB-Punkte des Bandfilters, wie bei Messfiltern
// ueblich. Im Menue umschaltbar.
const BandLimits& bandLimits(Band b);

struct Report {
  const char* band = "";
  float splFast = 0.0f;     // RMS, tau = 125 ms, sinc-korrigiert
  float splSlow = 0.0f;     // RMS, tau = 1 s, sinc-korrigiert
  float splMaxRms = 0.0f;   // groesster RMS seit dem letzten Reset
  float splPeak = 0.0f;     // True-Peak seit dem letzten Reset
  float sincCorrDb = 0.0f;  // angewandte Korrektur des Zeitbereichspfads
  float f0 = 0.0f;          // dominante Frequenz
  float splTone = 0.0f;     // Pegel dieses Tons, aus dem Spektrum
  float splBandFft = 0.0f;  // Bandpegel aus dem Spektrum (Parseval)
  float rawPressurePa = 0.0f;  // geglaetteter Absolutdruck, fuer Kalibrierung
  bool pressurePlausible = true;  // Rohdruck innerhalb der Sensorgrenzen
};

class Meter {
 public:
  // nFft: 512 fuer LIVE, 1024 fuer RTA. Muss Zweierpotenz <= kMaxFft sein.
  bool init(float fs, Band band, int nFft);

  // Bandwechsel zur Laufzeit. Setzt Filter und Haltewerte zurueck -- sonst
  // schleppt der Peak-Wert den Einschwingueberschwinger des neuen Filters mit.
  void setBand(Band band);
  Band band() const { return band_; }

  // Ein Rohsample [Pa] verarbeiten, gibt den bandgefilterten Wert zurueck.
  float process(float pRaw);

  // Haltewerte loeschen und die Einschwingsperre neu starten.
  void resetHold();

  Report report() const;

  // Geglaetteter Absolutdruck in Pa, also VOR dem DC-Blocker.
  //
  // Dies ist der Messwert fuer die statische Gain-Pruefung aus
  // docs/05-kalibrierung.md: Sensor in eine dichte Kammer, Kammer ueber ein
  // Wassersaeulen-Manometer mit bekanntem Ueberdruck beaufschlagen, und dieser
  // Wert muss um rho*g*h steigen. Ohne diesen Ausgang waere die Kalibrierung
  // nicht durchfuehrbar -- der DC-Blocker entfernt genau die Groesse, um die
  // es dabei geht.
  //
  // Zeitkonstante 2 s: bei der Kalibrierung zaehlt Ruhe, nicht Schnelligkeit.
  float rawPressurePa() const { return rawAvg_; }

  // Sinc-Korrektur fuer den Zeitbereichspfad, gebildet mit der dominanten
  // Frequenz. Bei Bassmessungen dominiert praktisch immer ein Ton; bei
  // breitbandigem Inhalt bleibt ein Restfehler unter 0,4 dB bis 100 Hz.
  float sincOffsetDb() const;

  const Spectrum& spectrum() const { return spectrum_; }
  // Letztes Betragsspektrum, magCount() Werte. Fuer die RTA-Anzeige.
  const float* magnitudes() const { return mags_; }
  bool spectrumReady() const { return specReady_; }

 private:
  void feedFft(float x);

  float fs_ = 0.0f;
  Band band_ = Band::k10to100;
  DcBlocker dc_;
  BandFilter filt_;
  LevelDetector fast_, slow_;
  Spectrum spectrum_;

  int guard_ = 0;  // verbleibende Samples der Einschwingsperre

  static constexpr float kRawTau = 2.0f;  // s, Glaettung des Absolutdrucks
  float rawAlpha_ = 0.0f;
  float rawAvg_ = 0.0f;
  bool rawPrimed_ = false;

  float ring_[kMaxFft];
  float block_[kMaxFft];
  float mags_[kMaxFft / 2 + 1];
  int writeIdx_ = 0, filled_ = 0, hop_ = 0;
  bool specReady_ = false;

  float f0_ = 0.0f, tonePa_ = 0.0f, bandPa_ = 0.0f;
};

}  // namespace vu
