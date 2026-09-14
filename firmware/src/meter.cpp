#include "meter.h"

namespace vu {

namespace {
constexpr BandLimits kBands[kBandCount] = {
    {10.0f, 100.0f, "10-100"},
    {5.0f, 150.0f, "5-150"},
};
}  // namespace

const BandLimits& bandLimits(Band b) {
  const int i = static_cast<int>(b);
  return kBands[(i >= 0 && i < kBandCount) ? i : 0];
}

bool Meter::init(float fs, Band band, int nFft) {
  if (fs <= 0.0f) return false;
  const BandLimits& bl = bandLimits(band);
  if (bl.hi > fs * kMaxBandFraction) return false;
  if (!spectrum_.init(nFft, fs)) return false;

  fs_ = fs;
  band_ = band;
  dc_.design(kDcBlockerFc, fs);
  filt_.design(bl.lo, bl.hi, fs);
  fast_.init(fs, kTauFast);
  slow_.init(fs, kTauSlow);
  writeIdx_ = filled_ = hop_ = 0;
  specReady_ = false;
  f0_ = tonePa_ = bandPa_ = 0.0f;
  resetHold();
  return true;
}

void Meter::setBand(Band band) {
  const BandLimits& bl = bandLimits(band);
  if (bl.hi > fs_ * kMaxBandFraction) return;
  band_ = band;
  filt_.design(bl.lo, bl.hi, fs_);
  filt_.reset();
  resetHold();
}

void Meter::resetHold() {
  fast_.resetHold();
  slow_.resetHold();
  guard_ = static_cast<int>(fs_ * kHoldGuardS);
}

float Meter::process(float pRaw) {
  // Die FFT bekommt das Signal VOR dem Bandfilter, nur DC-befreit. Sonst erbt
  // die Frequenzanzeige die -3 dB der Filterflanke und ein Ton genau auf einer
  // Bandgrenze wuerde 3 dB zu niedrig angezeigt.
  const float d = dc_.process(pRaw);
  const float x = filt_.process(d);

  const bool hold = guard_ <= 0;
  if (!hold) --guard_;
  fast_.process(x, hold);
  slow_.process(x, hold);

  feedFft(d);
  return x;
}

void Meter::feedFft(float x) {
  const int n = spectrum_.size();
  ring_[writeIdx_] = x;
  writeIdx_ = (writeIdx_ + 1) % n;
  if (filled_ < n) ++filled_;
  const int hopLen = n / 4;
  if (filled_ < n) return;
  if (specReady_ && ++hop_ < hopLen) return;
  hop_ = 0;

  // Ringpuffer in zeitlicher Reihenfolge auspacken
  for (int i = 0; i < n; ++i) block_[i] = ring_[(writeIdx_ + i) % n];
  spectrum_.magnitudes(block_, mags_);
  const BandLimits& bl = bandLimits(band_);
  spectrum_.dominant(mags_, bl.lo, bl.hi, &f0_, &tonePa_);
  bandPa_ = spectrum_.bandRms(mags_, bl.lo, bl.hi);
  specReady_ = true;
}

float Meter::sincOffsetDb() const {
  return f0_ > 0.0f ? sincCorrectionDb(f0_, fs_) : 0.0f;
}

Report Meter::report() const {
  const float corr = sincOffsetDb();
  Report r;
  r.band = bandLimits(band_).name;
  r.splFast = fast_.splRms() + corr;
  r.splSlow = slow_.splRms() + corr;
  r.splMaxRms = fast_.splMaxRms() + corr;
  r.splPeak = fast_.splPeak() + corr;
  r.sincCorrDb = corr;
  r.f0 = f0_;
  r.splTone = splFromPa(tonePa_);
  r.splBandFft = splFromPa(bandPa_);
  return r;
}

}  // namespace vu
