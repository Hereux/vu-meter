#include "spectrum.h"

#include <cmath>

#include "dsp.h"

namespace vu {

bool Spectrum::init(int n, float fs) {
  if (n < 8 || n > kMaxFft || (n & (n - 1)) != 0) return false;
  n_ = n;
  fs_ = fs;
  binHz_ = fs / static_cast<float>(n);
  // Periodisches Hann-Fenster (fuer Spektralanalyse korrekt, nicht symmetrisch)
  for (int i = 0; i < n; ++i) {
    window_[i] = 0.5f - 0.5f * std::cos(2.0f * static_cast<float>(M_PI) * i / n);
  }
  // Drehfaktoren einmal vorberechnen statt sie im Schmetterling
  // aufzumultiplizieren: in float32 wuerde sich der Fehler sonst ueber die
  // Stufen aufsummieren.
  for (int k = 0; k < n / 2; ++k) {
    const float ang = -2.0f * static_cast<float>(M_PI) * k / n;
    twRe_[k] = std::cos(ang);
    twIm_[k] = std::sin(ang);
  }
  return true;
}

void Spectrum::magnitudes(const float* block, float* mags) {
  const int n = n_;
  for (int i = 0; i < n; ++i) {
    re_[i] = block[i] * window_[i];
    im_[i] = 0.0f;
  }
  // Bit-Reversal-Permutation
  for (int i = 1, j = 0; i < n; ++i) {
    int bit = n >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j |= bit;
    if (i < j) {
      float t = re_[i]; re_[i] = re_[j]; re_[j] = t;
      t = im_[i]; im_[i] = im_[j]; im_[j] = t;
    }
  }
  for (int len = 2; len <= n; len <<= 1) {
    const int half = len >> 1;
    const int step = n / len;
    for (int i = 0; i < n; i += len) {
      for (int k = 0; k < half; ++k) {
        const float wr = twRe_[k * step], wi = twIm_[k * step];
        const float vr = re_[i + k + half] * wr - im_[i + k + half] * wi;
        const float vi = re_[i + k + half] * wi + im_[i + k + half] * wr;
        const float ur = re_[i + k], ui = im_[i + k];
        re_[i + k] = ur + vr;
        im_[i + k] = ui + vi;
        re_[i + k + half] = ur - vr;
        im_[i + k + half] = ui - vi;
      }
    }
  }
  for (int k = 0; k <= n / 2; ++k) {
    mags[k] = std::sqrt(re_[k] * re_[k] + im_[k] * im_[k]);
  }
}

float Spectrum::bandRms(const float* mags, float fLo, float fHi,
                        bool sincCorrect) const {
  int kLo = static_cast<int>(std::ceil(fLo / binHz_));
  int kHi = static_cast<int>(std::floor(fHi / binHz_));
  if (kLo < 1) kLo = 1;
  if (kHi > n_ / 2 - 1) kHi = n_ / 2 - 1;
  float acc = 0.0f;
  for (int k = kLo; k <= kHi; ++k) {
    float m = mags[k];
    if (sincCorrect) {
      m *= std::pow(10.0f, sincCorrectionDb(k * binHz_, fs_) / 20.0f);
    }
    acc += 2.0f * m * m;  // Faktor 2 fuer die gespiegelte negative Haelfte
  }
  const float nn = static_cast<float>(n_) * static_cast<float>(n_);
  return std::sqrt(acc / (nn * kWindowPower));
}

void Spectrum::dominant(const float* mags, float fLo, float fHi, float* f0,
                        float* pRms) const {
  // Suchbereich um ein Bin ueber die Bandgrenzen hinaus erweitern: ein Ton
  // genau auf der Bandgrenze hat sein Maximum sonst evtl. im ersten Bin
  // ausserhalb, die Interpolation liefe in die Begrenzung.
  int kLo = static_cast<int>(std::ceil(fLo / binHz_)) - 1;
  int kHi = static_cast<int>(std::floor(fHi / binHz_)) + 1;
  if (kLo < 1) kLo = 1;
  if (kHi > n_ / 2 - 1) kHi = n_ / 2 - 1;
  if (kHi <= kLo) {
    *f0 = 0.0f;
    *pRms = 0.0f;
    return;
  }
  int k0 = kLo;
  for (int k = kLo + 1; k <= kHi; ++k) {
    if (mags[k] > mags[k0]) k0 = k;
  }

  float delta = 0.0f;
  if (k0 >= 1 && k0 < n_ / 2) {
    const float eps = 1e-30f;
    const float a = std::log(mags[k0 - 1] + eps);
    const float b = std::log(mags[k0] + eps);
    const float c = std::log(mags[k0 + 1] + eps);
    const float denom = a - 2.0f * b + c;
    if (denom != 0.0f) {
      delta = 0.5f * (a - c) / denom;
      if (delta < -0.5f) delta = -0.5f;
      if (delta > 0.5f) delta = 0.5f;
    }
  }
  *f0 = (static_cast<float>(k0) + delta) * binHz_;

  int lo = k0 - 2, hi = k0 + 2;
  if (lo < 1) lo = 1;
  if (hi > n_ / 2 - 1) hi = n_ / 2 - 1;
  float acc = 0.0f;
  for (int k = lo; k <= hi; ++k) acc += 2.0f * mags[k] * mags[k];
  const float nn = static_cast<float>(n_) * static_cast<float>(n_);
  float rms = std::sqrt(acc / (nn * kWindowPower));
  rms *= std::pow(10.0f, sincCorrectionDb(*f0, fs_) / 20.0f);
  *pRms = rms;
}

}  // namespace vu
