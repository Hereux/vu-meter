// Abnahmetest der C++-Portierung gegen die Referenzwerte aus dem Python-Modell.
//
// Laeuft ohne Hardware:
//   make -C firmware test      (oder: pio test -e native)
//
// Die Sollwerte in reference_vectors.h stammen aus tools/dsp_model.py und sind
// dort durch 99 eigene Tests abgesichert. Damit ist die Portierung eine
// Uebersetzung mit objektivem Abnahmekriterium und keine zweite
// Implementierung, der man wieder neu glauben muss.

#include <cmath>
#include <cstdio>
#include <cstring>

#include "../../src/dsp.h"
#include "../../src/meter.h"
#include "../../src/spectrum.h"
#include "../reference_vectors.h"

namespace {

int g_total = 0;
int g_failed = 0;

void check(bool ok, const char* name, const char* fmt = nullptr, ...) {
  (void)fmt;
  ++g_total;
  if (!ok) ++g_failed;
  std::printf("  [%s] %s\n", ok ? "OK " : "FAIL", name);
}

void checkNear(float got, float want, float tol, const char* name) {
  const bool ok = std::fabs(got - want) <= tol;
  ++g_total;
  if (!ok) ++g_failed;
  std::printf("  [%s] %-58s ist %.6g, soll %.6g (+/-%.3g)\n",
              ok ? "OK " : "FAIL", name, got, want, tol);
}

// Sinus mit nachgebildeter Sensormittelung, identisch zu
// dsp_model.sine(..., sensor_average=True).
float sampleSine(int i, float fs, float freq, float splDb, float offsetPa,
                 bool sensorAverage, float phase = 0.0f) {
  float amp = vu::paFromSpl(splDb) * std::sqrt(2.0f);
  float tShift = 0.0f;
  if (sensorAverage) {
    const float x = static_cast<float>(M_PI) * freq / fs;
    if (x != 0.0f) amp *= std::sin(x) / x;
    tShift = 0.5f / fs;
  }
  const float t = static_cast<float>(i) / fs;
  return offsetPa + amp * std::sin(2.0f * static_cast<float>(M_PI) * freq *
                                       (t + tShift) + phase);
}

void runSine(vu::Meter& m, float fs, float seconds, float freq, float splDb,
             float offsetPa = 101325.0f, bool sensorAverage = true,
             float phase = 0.0f) {
  const int n = static_cast<int>(fs * seconds + 0.5f);
  for (int i = 0; i < n; ++i) {
    m.process(sampleSine(i, fs, freq, splDb, offsetPa, sensorAverage, phase));
  }
}

// ------------------------------------------------------------------ Tests

void testButterworthQ() {
  std::printf("\nFilterauslegung\n");
  float q[2];
  vu::butterworthQ(4, q);
  checkNear(q[0], ref::kButterQ4[0], 1e-6f, "Guetefaktor Sektion 1");
  checkNear(q[1], ref::kButterQ4[1], 1e-6f, "Guetefaktor Sektion 2");
}

void compareBand(const char* label, float lo, float hi, float fs,
                 const ref::BiquadCoeffs* want, const float* wantResp) {
  vu::BandFilter bf;
  bf.design(lo, hi, fs);
  char name[128];
  for (int i = 0; i < vu::BandFilter::kSections; ++i) {
    std::snprintf(name, sizeof(name), "%s Sektion %d b0", label, i);
    checkNear(bf.sections[i].b0, want[i].b0, 1e-6f, name);
    std::snprintf(name, sizeof(name), "%s Sektion %d b1", label, i);
    checkNear(bf.sections[i].b1, want[i].b1, 1e-6f, name);
    std::snprintf(name, sizeof(name), "%s Sektion %d a1", label, i);
    checkNear(bf.sections[i].a1, want[i].a1, 1e-6f, name);
    std::snprintf(name, sizeof(name), "%s Sektion %d a2", label, i);
    checkNear(bf.sections[i].a2, want[i].a2, 1e-6f, name);
  }
  const float probe[6] = {5.0f, 10.0f, 20.0f, 50.0f, 100.0f, 150.0f};
  for (int i = 0; i < 6; ++i) {
    std::snprintf(name, sizeof(name), "%s Frequenzgang bei %.0f Hz", label,
                  probe[i]);
    checkNear(bf.responseDb(probe[i]), wantResp[i], 0.01f, name);
  }
}

void testBandCoefficients() {
  std::printf("\nBandfilter gegen die Referenzkoeffizienten\n");
  compareBand("10-100 @500Hz", 10.0f, 100.0f, 500.0f, ref::kBand10_100_fs500,
              ref::kBand10_100_fs500_respDb);
  compareBand("5-150 @500Hz", 5.0f, 150.0f, 500.0f, ref::kBand5_150_fs500,
              ref::kBand5_150_fs500_respDb);
  compareBand("10-100 @622Hz", 10.0f, 100.0f, 622.0f, ref::kBand10_100_fs622,
              ref::kBand10_100_fs622_respDb);
  compareBand("5-150 @622Hz", 5.0f, 150.0f, 622.0f, ref::kBand5_150_fs622,
              ref::kBand5_150_fs622_respDb);
}

void testGoldenVector() {
  std::printf("\nGoldener Vektor der Gesamtkette\n");
  vu::Meter m;
  check(m.init(ref::kGoldenFs, vu::Band::k10to100, 512), "Meter laesst sich anlegen");
  float worst = 0.0f;
  int worstIdx = -1;
  for (int i = 0; i < ref::kGoldenLen; ++i) {
    const float got = m.process(ref::kGoldenIn[i]);
    const float err = std::fabs(got - ref::kGoldenOut[i]);
    if (err > worst) { worst = err; worstIdx = i; }
  }
  // Absoluttoleranz in Pa. Die Signalamplitude liegt bei ~894 Pa, 0,05 Pa
  // entsprechen 0,0005 dB.
  const bool ok = worst < 0.05f;
  ++g_total;
  if (!ok) ++g_failed;
  std::printf("  [%s] 64 Ausgangswerte stimmen mit dem Modell ueberein "
              "(max. Abweichung %.4g Pa bei Index %d)\n",
              ok ? "OK " : "FAIL", worst, worstIdx);
}

void testToneCases() {
  std::printf("\nStationaere Sollwerte, Sinus 150 dB, Band 10-100, fs = 622 Hz\n");
  char name[128];
  for (int i = 0; i < ref::kToneCaseLen; ++i) {
    const ref::ToneCase& tc = ref::kToneCases[i];
    vu::Meter m;
    m.init(ref::kGoldenFs, vu::Band::k10to100, 1024);
    runSine(m, ref::kGoldenFs, 8.0f, tc.freq, 150.0f);
    const vu::Report r = m.report();
    std::snprintf(name, sizeof(name), "%5.1f Hz: Tonpegel", tc.freq);
    checkNear(r.splTone, tc.splTone, 0.05f, name);
    std::snprintf(name, sizeof(name), "%5.1f Hz: Detektor Slow", tc.freq);
    checkNear(r.splSlow, tc.splSlow, 0.05f, name);
    std::snprintf(name, sizeof(name), "%5.1f Hz: dominante Frequenz", tc.freq);
    checkNear(r.f0, tc.f0, 0.02f, name);
  }
}

void testHoldGuard() {
  std::printf("\nEinschwingsperre und Peak-Hold\n");
  const float fs = ref::kGoldenFs;
  vu::Meter m;
  m.init(fs, vu::Band::k10to100, 512);
  // Erst einschwingen, dann scharf schalten -- so arbeitet der PEAK-Modus:
  // Taster druecken, dann den Burp fahren.
  runSine(m, fs, 2.0f, 40.0f, 150.0f);
  m.resetHold();
  runSine(m, fs, 6.0f, 40.0f, 150.0f, 101325.0f, true,
          static_cast<float>(M_PI) / 3.0f);
  const vu::Report r = m.report();
  checkNear(r.splPeak - r.splSlow, 3.01f, 0.1f,
            "True-Peak liegt 3,01 dB ueber RMS");
  checkNear(r.splMaxRms, r.splFast, 0.2f, "max. RMS entspricht dem aktuellen RMS");

  // Ohne Sperre wuerde der Filterueberschwinger den Peak verfaelschen.
  vu::Meter m2;
  m2.init(fs, vu::Band::k10to100, 512);
  runSine(m2, fs, 2.0f, 40.0f, 150.0f);
  const vu::Report r2 = m2.report();
  const bool ok = r2.splPeak < 154.0f;
  ++g_total;
  if (!ok) ++g_failed;
  std::printf("  [%s] Sperre haelt den Ueberschwinger aus dem Peak-Hold "
              "(%.2f dB, Sollspitze 153,01 dB)\n", ok ? "OK " : "FAIL",
              r2.splPeak);
}

void testLinearity() {
  std::printf("\nLinearitaet ueber den Messbereich\n");
  char name[64];
  const float levels[] = {110.0f, 120.0f, 130.0f, 140.0f, 150.0f, 160.0f,
                          170.0f, 178.0f};
  for (float spl : levels) {
    vu::Meter m;
    m.init(ref::kGoldenFs, vu::Band::k10to100, 1024);
    runSine(m, ref::kGoldenFs, 8.0f, 40.0f, spl);
    std::snprintf(name, sizeof(name), "%.0f dB SPL", spl);
    checkNear(m.report().splTone, spl, 0.1f, name);
  }
}

void testBandSwitching() {
  std::printf("\nBandumschaltung\n");
  const float fs = ref::kGoldenFs;
  vu::Meter m;
  m.init(fs, vu::Band::k10to100, 512);
  check(std::strcmp(m.report().band, "10-100") == 0, "Startband ist 10-100");
  m.setBand(vu::Band::k5to150);
  check(std::strcmp(m.report().band, "5-150") == 0, "Umschaltung auf 5-150");

  // 130 Hz: im Band 5-150 drin, im Band 10-100 draussen.
  for (int b = 0; b < vu::kBandCount; ++b) {
    vu::Meter mm;
    mm.init(fs, static_cast<vu::Band>(b), 512);
    runSine(mm, fs, 8.0f, 130.0f, 150.0f);
    vu::BandFilter bf;
    const vu::BandLimits& bl = vu::bandLimits(static_cast<vu::Band>(b));
    bf.design(bl.lo, bl.hi, fs);
    // Rohwert des Detektors, also ohne die f0-basierte Sinc-Korrektur
    const float raw = mm.report().splSlow - mm.report().sincCorrDb;
    const float want = 150.0f + bf.responseDb(130.0f) -
                       vu::sincCorrectionDb(130.0f, fs);
    char name[96];
    std::snprintf(name, sizeof(name), "Band %s: 130 Hz trifft den Sollpegel",
                  bl.name);
    checkNear(raw, want, 0.15f, name);
  }

  // Abtastrate zu niedrig fuer das Band: muss abgelehnt werden, statt still
  // ein entartetes Filter zu bauen.
  vu::Meter slow;
  check(!slow.init(200.0f, vu::Band::k5to150, 512),
        "fs = 200 Hz mit Band 5-150 wird abgelehnt");
  check(!slow.init(200.0f, vu::Band::k10to100, 512),
        "fs = 200 Hz mit Band 10-100 wird abgelehnt (100 Hz laege auf Nyquist)");
  vu::Meter ok300;
  check(ok300.init(300.0f, vu::Band::k10to100, 512),
        "fs = 300 Hz mit Band 10-100 wird angenommen");
  vu::Meter bmp581;
  check(bmp581.init(500.0f, vu::Band::k5to150, 512),
        "fs = 500 Hz mit Band 5-150 wird angenommen");
  // Ein abgelehnter Bandwechsel darf das laufende Band nicht veraendern.
  vu::Meter keep;
  keep.init(400.0f, vu::Band::k10to100, 512);
  keep.setBand(vu::Band::k5to150);  // 150 > 0,4 * 400 = 160? nein, zulaessig
  check(std::strcmp(keep.report().band, "5-150") == 0,
        "zulaessiger Bandwechsel wird uebernommen");
  vu::Meter keep2;
  keep2.init(330.0f, vu::Band::k10to100, 512);
  keep2.setBand(vu::Band::k5to150);  // 150 > 0,4 * 330 = 132 -> abgelehnt
  check(std::strcmp(keep2.report().band, "10-100") == 0,
        "unzulaessiger Bandwechsel laesst das alte Band stehen");
}

void testRobustness() {
  std::printf("\nUnterdrueckung von Luftdruck und Drift\n");
  const float fs = ref::kGoldenFs;
  vu::Meter ref0;
  ref0.init(fs, vu::Band::k10to100, 1024);
  runSine(ref0, fs, 8.0f, 40.0f, 150.0f);
  const float base = ref0.report().splTone;

  const float offsets[] = {85000.0f, 101325.0f, 108000.0f};
  char name[80];
  for (float off : offsets) {
    vu::Meter m;
    m.init(fs, vu::Band::k10to100, 1024);
    runSine(m, fs, 8.0f, 40.0f, 150.0f, off);
    std::snprintf(name, sizeof(name), "Luftdruck %.0f hPa aendert den Pegel nicht",
                  off / 100.0f);
    checkNear(m.report().splTone, base, 0.05f, name);
  }
}

void testFftSanity() {
  std::printf("\nFFT-Grundpruefungen\n");
  vu::Spectrum sp;
  check(!sp.init(1000, 622.0f), "Nicht-Zweierpotenz wird abgelehnt");
  check(!sp.init(2048, 622.0f), "Blocklaenge ueber kMaxFft wird abgelehnt");
  check(sp.init(1024, 622.0f), "1024 Punkte werden angenommen");
  checkNear(sp.binHz(), 622.0f / 1024.0f, 1e-5f, "Bin-Breite");

  // Zwei gleich starke Toene ergeben +3,01 dB Bandpegel.
  static float blk[1024];
  static float mags[513];
  const float amp = vu::paFromSpl(150.0f) * std::sqrt(2.0f);
  for (int i = 0; i < 1024; ++i) {
    const float t = static_cast<float>(i) / 622.0f;
    blk[i] = amp * (std::sin(2.0f * static_cast<float>(M_PI) * 30.0f * t) +
                    std::sin(2.0f * static_cast<float>(M_PI) * 70.0f * t));
  }
  sp.magnitudes(blk, mags);
  checkNear(vu::splFromPa(sp.bandRms(mags, 10.0f, 100.0f, false)), 153.01f, 0.1f,
            "zwei gleich starke Toene ergeben +3,01 dB");
}

}  // namespace

int main() {
  std::printf("Abnahmetest der C++-Portierung gegen das Python-Referenzmodell\n");
  testButterworthQ();
  testBandCoefficients();
  testGoldenVector();
  testToneCases();
  testHoldGuard();
  testLinearity();
  testBandSwitching();
  testRobustness();
  testFftSanity();
  std::printf("\n%d von %d Tests bestanden\n", g_total - g_failed, g_total);
  return g_failed == 0 ? 0 : 1;
}
