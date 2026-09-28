// Spektralpfad: Hann-Fenster, Radix-2-FFT, dominante Frequenz, Bandpegel.
//
// Portierung von tools/dsp_model.py. Kein ESP-DSP noetig: bei N = 1024 und
// Hop N/4 laeuft die FFT nur 2,4-mal pro Sekunde, das kostet auf dem ESP32
// unter 1 % Rechenzeit.
#pragma once

namespace vu {

// Groesster unterstuetzter FFT-Block. 1024 Punkte bei fs = 622 Hz ergeben
// 0,61 Hz Aufloesung bei 1,65 s Fensterlaenge.
constexpr int kMaxFft = 1024;

class Spectrum {
 public:
  // n muss eine Zweierpotenz <= kMaxFft sein. Gibt false zurueck, wenn nicht.
  bool init(int n, float fs);

  int size() const { return n_; }
  float binHz() const { return binHz_; }
  int magCount() const { return n_ / 2 + 1; }

  // Einseitiges Betragsspektrum. mags muss magCount() Werte fassen.
  void magnitudes(const float* block, float* mags);

  // Effektivdruck im Band ueber Parseval. Rechteckige Bandgrenzen, also ohne
  // den Flankeneinfluss eines Zeitbereichsfilters.
  float bandRms(const float* mags, float fLo, float fHi,
                bool sincCorrect = true) const;

  // Staerkster Anteil im Band. f0 per parabolischer Interpolation ueber die
  // logarithmierten Betraege, Amplitude ueber die Hauptkeule (Peak +/- 2 Bins).
  void dominant(const float* mags, float fLo, float fHi, float* f0,
                float* pRms) const;

 private:
  // Leistungskorrektur des Hann-Fensters: mean(w^2) = 3/8
  static constexpr float kWindowPower = 0.375f;

  int n_ = 0;
  float fs_ = 0.0f;
  float binHz_ = 0.0f;
  float window_[kMaxFft];
  float twRe_[kMaxFft / 2];  // vorberechnete Drehfaktoren
  float twIm_[kMaxFft / 2];
  float re_[kMaxFft];
  float im_[kMaxFft];
};

}  // namespace vu
