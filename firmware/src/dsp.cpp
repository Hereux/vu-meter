#include "dsp.h"

#include <cmath>

namespace vu {

float splFromPa(float pRms) {
  if (pRms <= 0.0f) return -INFINITY;
  return 20.0f * std::log10(pRms / kPRef);
}

float paFromSpl(float splDb) { return kPRef * std::pow(10.0f, splDb / 20.0f); }

float sincCorrectionDb(float f, float fs) {
  const float x = static_cast<float>(M_PI) * f / fs;
  if (x == 0.0f) return 0.0f;
  return -20.0f * std::log10(std::sin(x) / x);
}

void butterworthQ(int order, float* q) {
  const int n = order / 2;
  for (int k = 0; k < n; ++k) {
    q[k] = 1.0f / (2.0f * std::cos((2 * k + 1) * static_cast<float>(M_PI) /
                                   (2 * order)));
  }
}

Biquad Biquad::lowpass(float fc, float fs, float q) {
  const float w0 = 2.0f * static_cast<float>(M_PI) * fc / fs;
  const float cw = std::cos(w0), sw = std::sin(w0);
  const float alpha = sw / (2.0f * q);
  const float a0 = 1.0f + alpha;
  Biquad b;
  b.b0 = ((1.0f - cw) * 0.5f) / a0;
  b.b1 = (1.0f - cw) / a0;
  b.b2 = b.b0;
  b.a1 = (-2.0f * cw) / a0;
  b.a2 = (1.0f - alpha) / a0;
  return b;
}

Biquad Biquad::highpass(float fc, float fs, float q) {
  const float w0 = 2.0f * static_cast<float>(M_PI) * fc / fs;
  const float cw = std::cos(w0), sw = std::sin(w0);
  const float alpha = sw / (2.0f * q);
  const float a0 = 1.0f + alpha;
  Biquad b;
  b.b0 = ((1.0f + cw) * 0.5f) / a0;
  b.b1 = (-(1.0f + cw)) / a0;
  b.b2 = b.b0;
  b.a1 = (-2.0f * cw) / a0;
  b.a2 = (1.0f - alpha) / a0;
  return b;
}

float Biquad::magnitudeDb(float f, float fs) const {
  const float w = -2.0f * static_cast<float>(M_PI) * f / fs;
  const float cr = std::cos(w), ci = std::sin(w);
  // z = e^{jw}, z^2
  const float c2r = cr * cr - ci * ci, c2i = 2.0f * cr * ci;
  const float nr = b0 + b1 * cr + b2 * c2r;
  const float ni = b1 * ci + b2 * c2i;
  const float dr = 1.0f + a1 * cr + a2 * c2r;
  const float di = a1 * ci + a2 * c2i;
  const float mag = std::sqrt((nr * nr + ni * ni) / (dr * dr + di * di));
  return mag > 0.0f ? 20.0f * std::log10(mag) : -INFINITY;
}

void DcBlocker::design(float fc, float fs) {
  a_ = std::exp(-2.0f * static_cast<float>(M_PI) * fc / fs);
  reset();
}

float DcBlocker::process(float x) {
  if (!primed_) {
    // Erster Wert definiert den Arbeitspunkt, sonst schwingt der Filter nach
    // dem Einschalten ueber 100 kPa aus.
    x1_ = x;
    primed_ = true;
  }
  const float y = a_ * (y1_ + x - x1_);
  x1_ = x;
  y1_ = y;
  return y;
}

void BandFilter::design(float lo, float hi, float sampleRate) {
  fLo = lo;
  fHi = hi;
  fs = sampleRate;
  float q[2];
  butterworthQ(4, q);
  sections[0] = Biquad::highpass(lo, sampleRate, q[0]);
  sections[1] = Biquad::highpass(lo, sampleRate, q[1]);
  sections[2] = Biquad::lowpass(hi, sampleRate, q[0]);
  sections[3] = Biquad::lowpass(hi, sampleRate, q[1]);
}

void BandFilter::reset() {
  for (int i = 0; i < kSections; ++i) sections[i].reset();
}

float BandFilter::process(float x) {
  for (int i = 0; i < kSections; ++i) x = sections[i].process(x);
  return x;
}

float BandFilter::responseDb(float f) const {
  float sum = 0.0f;
  for (int i = 0; i < kSections; ++i) sum += sections[i].magnitudeDb(f, fs);
  return sum;
}

void LevelDetector::init(float fs, float tau) {
  alpha_ = 1.0f - std::exp(-1.0f / (fs * tau));
  ms_ = 0.0f;
  resetHold();
}

float LevelDetector::process(float x, bool hold) {
  ms_ += alpha_ * (x * x - ms_);
  const float r = std::sqrt(ms_);
  if (hold) {
    if (r > maxRms_) maxRms_ = r;
    const float a = std::fabs(x);
    if (a > peak_) peak_ = a;
  }
  return r;
}

float LevelDetector::rms() const { return std::sqrt(ms_); }

}  // namespace vu
