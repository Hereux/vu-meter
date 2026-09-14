#!/usr/bin/env python3
"""Erzeugt die Referenzwerte fuer die C++-Portierung (Phase 3).

Schreibt firmware/test/reference_vectors.h. Der C++-Unittest vergleicht seine
eigene Filterauslegung und seinen Signalpfad gegen diese Werte; damit ist
sichergestellt, dass die Portierung rechnerisch identisch zum hier getesteten
Python-Modell ist.

Absichtlich keine fertige Koeffiziententabelle fuer die Firmware: die
tatsaechliche Abtastrate wird beim Start gemessen (Sensor-RC-Oszillator, +/-5 %)
und die Koeffizienten werden daraus zur Laufzeit berechnet. Feste Tabellen
waeren um genau diesen Fehler daneben.

Aufruf:  python3 tools/export_reference.py
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import dsp_model as d

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                   "firmware", "test", "reference_vectors.h")

FS_CASES = (500.0, 622.0)
N_GOLDEN = 64          # Laenge der Ein-/Ausgangsvektoren
GOLDEN_FS = 622.0
GOLDEN_FREQ = 40.0
GOLDEN_SPL = 150.0


def fmt(v):
    return f"{v:.12g}f"


def main():
    lines = []
    w = lines.append
    w("// Automatisch erzeugt von tools/export_reference.py -- nicht von Hand aendern.")
    w("// Referenzwerte aus dem Python-Modell (tools/dsp_model.py), abgesichert")
    w("// durch tools/test_dsp_model.py.")
    w("#pragma once")
    w("")
    w("namespace ref {")
    w("")
    w("// Guetefaktoren der Butterworth-Sektionen 4. Ordnung")
    qs = d.butterworth_q(4)
    w(f"constexpr float kButterQ4[2] = {{{fmt(qs[0])}, {fmt(qs[1])}}};")
    w("")
    w("struct BiquadCoeffs { float b0, b1, b2, a1, a2; };")
    w("")

    for fs in FS_CASES:
        tag = f"{int(round(fs))}"
        for band, (lo, hi) in d.BANDS.items():
            bname = band.replace("-", "_")
            bf = d.BandFilter(lo, hi, fs)
            w(f"// Band {band} Hz bei fs = {fs:.0f} Hz: 2x Hochpass, 2x Tiefpass")
            w(f"constexpr BiquadCoeffs kBand{bname}_fs{tag}[4] = {{")
            for s in bf.sections:
                w(f"    {{{fmt(s.b0)}, {fmt(s.b1)}, {fmt(s.b2)}, "
                  f"{fmt(s.a1)}, {fmt(s.a2)}}},")
            w("};")
            w(f"// Sollfrequenzgang [dB] bei 5, 10, 20, 50, 100, 150 Hz")
            resp = [bf.response_db(f) for f in (5, 10, 20, 50, 100, 150)]
            w(f"constexpr float kBand{bname}_fs{tag}_respDb[6] = "
              f"{{{', '.join(fmt(v) for v in resp)}}};")
            w("")

    w("// Goldener Vektor: erste 64 Ausgangswerte der Gesamtkette")
    w(f"// fs = {GOLDEN_FS:.0f} Hz, Band 10-100, Sinus {GOLDEN_FREQ:.0f} Hz bei "
      f"{GOLDEN_SPL:.0f} dB SPL")
    w(f"// auf {101325.0:.0f} Pa Luftdruck, mit nachgebildeter Sensormittelung.")
    m = d.Meter(GOLDEN_FS, band="10-100")
    src = d.sine(GOLDEN_FS, N_GOLDEN / GOLDEN_FS, GOLDEN_FREQ, GOLDEN_SPL,
                 sensor_average=True)
    inp, out = [], []
    for p in src:
        inp.append(p)
        out.append(m.process(p))
    w(f"constexpr int kGoldenLen = {len(inp)};")
    w(f"constexpr float kGoldenFs = {fmt(GOLDEN_FS)};")
    w("constexpr float kGoldenIn[kGoldenLen] = {")
    for i in range(0, len(inp), 4):
        w("    " + ", ".join(fmt(v) for v in inp[i:i + 4]) + ",")
    w("};")
    w("constexpr float kGoldenOut[kGoldenLen] = {")
    for i in range(0, len(out), 4):
        w("    " + ", ".join(fmt(v) for v in out[i:i + 4]) + ",")
    w("};")
    w("")

    w("// Stationaere Sollwerte der Gesamtkette: Sinus 150 dB, Band 10-100,")
    w("// fs = 622 Hz, 8 s Laufzeit, mit Sensormittelung.")
    w("struct ToneCase { float freq, splTone, splSlow, f0; };")
    cases = []
    for f in (10.0, 20.0, 31.5, 50.0, 63.0, 80.0, 100.0):
        mm = d.Meter(GOLDEN_FS, band="10-100", n_fft=1024)
        mm.run(d.sine(GOLDEN_FS, 8.0, f, 150.0, sensor_average=True))
        r = mm.report()
        cases.append((f, r["spl_tone"], r["spl_slow"], r["f0"]))
    w(f"constexpr int kToneCaseLen = {len(cases)};")
    w("constexpr ToneCase kToneCases[kToneCaseLen] = {")
    for f, st, ss, f0 in cases:
        w(f"    {{{fmt(f)}, {fmt(st)}, {fmt(ss)}, {fmt(f0)}}},")
    w("};")
    w("")
    w(f"constexpr float kTauFast = {fmt(d.TAU_FAST)};")
    w(f"constexpr float kTauSlow = {fmt(d.TAU_SLOW)};")
    w(f"constexpr float kHoldGuardS = {fmt(d.HOLD_GUARD)};")
    w(f"constexpr float kDcBlockerFc = {fmt(0.5)};")
    w("")
    w("}  // namespace ref")
    w("")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as fh:
        fh.write("\n".join(lines))
    print(f"geschrieben: {os.path.relpath(OUT)}  ({len(lines)} Zeilen)")


if __name__ == "__main__":
    main()
