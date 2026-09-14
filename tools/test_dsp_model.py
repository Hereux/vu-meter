#!/usr/bin/env python3
"""Abnahmetests fuer das DSP-Referenzmodell (Phase 1).

Ohne Fremdbibliotheken lauffaehig:  python3 tools/test_dsp_model.py
Exitcode 0 = alle Tests bestanden.

Dieselben Testfaelle werden spaeter in test/test_dsp.cpp gegen die
C++-Portierung gefahren. Die hier erzeugten Sollwerte sind die Referenz.
"""

import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import dsp_model as d

FS_LIST = (500.0, 622.0)
_results = []


def check(name, ok, detail=""):
    _results.append((name, ok, detail))
    print(f"  [{'OK ' if ok else 'FAIL'}] {name}" + (f"  -- {detail}" if detail else ""))
    return ok


def close(a, b, tol):
    return abs(a - b) <= tol


# ------------------------------------------------------------------ Filter

def test_butterworth_q():
    print("\nFilterauslegung")
    qs = d.butterworth_q(4)
    check("Butterworth-Guetefaktoren 4. Ordnung",
          close(qs[0], 0.541196, 1e-5) and close(qs[1], 1.306563, 1e-5),
          f"Q = {qs[0]:.6f}, {qs[1]:.6f}")
    check("Butterworth 2. Ordnung ist Q = 1/sqrt(2)",
          close(d.butterworth_q(2)[0], 0.7071068, 1e-6))
    try:
        d.butterworth_q(3)
        check("ungerade Ordnung wird abgelehnt", False)
    except ValueError:
        check("ungerade Ordnung wird abgelehnt", True)


def test_band_response():
    print("\nBandfilter-Frequenzgang")
    for fs in FS_LIST:
        for band, (lo, hi) in d.BANDS.items():
            bf = d.BandFilter(lo, hi, fs)
            check(f"fs={fs:.0f} {band}: -3 dB an der unteren Grenze",
                  close(bf.response_db(lo), -3.01, 0.05), f"{bf.response_db(lo):.2f} dB")
            check(f"fs={fs:.0f} {band}: -3 dB an der oberen Grenze",
                  close(bf.response_db(hi), -3.01, 0.05), f"{bf.response_db(hi):.2f} dB")
            f_mid = math.sqrt(lo * hi)
            check(f"fs={fs:.0f} {band}: Durchlassbereich flach bei {f_mid:.0f} Hz",
                  abs(bf.response_db(f_mid)) < 0.05, f"{bf.response_db(f_mid):+.3f} dB")
            # 24 dB/Oktave unterhalb der unteren Grenze
            slope = bf.response_db(lo) - bf.response_db(lo / 2.0)
            check(f"fs={fs:.0f} {band}: Flanke ~24 dB/Oktave unten",
                  20.0 < slope < 26.0, f"{slope:.1f} dB/Oktave")

    bf = d.BandFilter(10.0, 100.0, 622.0)
    check("Band 10-100 sperrt 150 Hz um mehr als 15 dB",
          bf.response_db(150.0) < -15.0, f"{bf.response_db(150.0):.1f} dB")
    try:
        d.BandFilter(5.0, 150.0, 200.0)
        check("obere Grenze oberhalb Nyquist wird abgelehnt", False)
    except ValueError:
        check("obere Grenze oberhalb Nyquist wird abgelehnt", True,
              "BMP390 mit 200 Hz kann Band 5-150 nicht")


# ------------------------------------------------------------- Spektralpfad

def test_tone_accuracy():
    print("\nTonpegel und Frequenz (Spektralpfad)")
    for fs in FS_LIST:
        for f in (10.0, 12.5, 20.0, 31.5, 40.0, 50.0, 63.0, 80.0, 100.0):
            m = d.Meter(fs, band="10-100", n_fft=1024)
            m.run(d.sine(fs, 8.0, f, 150.0, sensor_average=True))
            r = m.report()
            check(f"fs={fs:.0f} {f:>5.1f} Hz: Pegelfehler < 0.1 dB",
                  close(r["spl_tone"], 150.0, 0.1), f"{r['spl_tone']-150.0:+.3f} dB")
            check(f"fs={fs:.0f} {f:>5.1f} Hz: Frequenzfehler < 0.1 Hz",
                  close(r["f0"], f, 0.1), f"{r['f0']-f:+.4f} Hz")


def test_interpolation_bias():
    print("\nPeak-Interpolation")
    fs, n = 622.0, 1024
    sp = d.Spectrum(n, fs)
    amp = d.pa_from_spl(150.0) * math.sqrt(2.0)
    worst = 0.0
    for off in (-0.5, -0.4, -0.25, -0.1, 0.0, 0.1, 0.25, 0.4, 0.5):
        k = 100 + off
        f = k * sp.bin_hz
        blk = [amp * math.sin(2.0 * math.pi * f * i / fs) for i in range(n)]
        f0, _ = sp.dominant(sp.magnitudes(blk), 5.0, 250.0)
        worst = max(worst, abs(f0 / sp.bin_hz - k))
    check("Bias der parabolischen Interpolation < 0.02 Bin",
          worst < 0.02, f"max {worst:.4f} Bin = {worst*sp.bin_hz:.4f} Hz")


def test_parseval():
    print("\nBandpegel aus dem Spektrum (Parseval)")
    fs, n = 622.0, 1024
    sp = d.Spectrum(n, fs)
    amp = d.pa_from_spl(150.0) * math.sqrt(2.0)
    blk = [amp * math.sin(2.0 * math.pi * 40.0 * i / fs) for i in range(n)]
    mags = sp.magnitudes(blk)
    lvl = d.spl_from_pa(sp.band_rms(mags, 10.0, 100.0, sinc_correct=False))
    check("Einzelton: Bandpegel = Tonpegel", close(lvl, 150.0, 0.1),
          f"{lvl:.3f} dB")
    # Zwei gleich starke Toene -> +3.01 dB
    blk2 = [amp * (math.sin(2.0*math.pi*30.0*i/fs) + math.sin(2.0*math.pi*70.0*i/fs))
            for i in range(n)]
    lvl2 = d.spl_from_pa(sp.band_rms(sp.magnitudes(blk2), 10.0, 100.0, sinc_correct=False))
    check("zwei gleich starke Toene ergeben +3.01 dB",
          close(lvl2, 153.01, 0.1), f"{lvl2:.3f} dB")


# ---------------------------------------------------------- Detektorpfad

def test_detector_accuracy():
    print("\nDetektorpfad im Durchlassbereich")
    for fs in FS_LIST:
        for f in (20.0, 31.5, 50.0, 63.0):
            m = d.Meter(fs, band="10-100")
            m.run(d.sine(fs, 8.0, f, 150.0, sensor_average=True))
            r = m.report()
            check(f"fs={fs:.0f} {f:>5.1f} Hz: Detektor Slow < 0.15 dB Fehler",
                  close(r["spl_slow"], 150.0, 0.15), f"{r['spl_slow']-150.0:+.3f} dB")


def test_peak_vs_rms():
    print("\nPeak- und RMS-Wert")
    fs = 622.0
    m = d.Meter(fs, band="10-100")
    # Erst einschwingen lassen, dann die Haltewerte scharf schalten -- genau so
    # arbeitet der PEAK-Modus im Geraet: Taster druecken, dann den Burp fahren.
    m.run(d.sine(fs, 2.0, 40.0, 150.0, sensor_average=True))
    m.reset_hold()
    m.run(d.sine(fs, 6.0, 40.0, 150.0, sensor_average=True, phase=math.pi / 3))
    r = m.report()
    check("Sinus: True-Peak liegt 3.01 dB ueber RMS",
          close(r["spl_peak"] - r["spl_slow"], 3.01, 0.1),
          f"{r['spl_peak']-r['spl_slow']:+.3f} dB")
    check("max. RMS entspricht dem stationaeren RMS",
          close(r["spl_max_rms"], r["spl_fast"], 0.2),
          f"max {r['spl_max_rms']:.2f} vs. aktuell {r['spl_fast']:.2f} dB")


def test_linearity():
    print("\nLinearitaet ueber den Messbereich")
    fs = 622.0
    for spl in (110.0, 120.0, 130.0, 140.0, 150.0, 160.0, 170.0, 178.0):
        m = d.Meter(fs, band="10-100", n_fft=1024)
        m.run(d.sine(fs, 8.0, 40.0, spl, sensor_average=True))
        r = m.report()
        check(f"{spl:>5.1f} dB: Fehler < 0.1 dB", close(r["spl_tone"], spl, 0.1),
              f"{r['spl_tone']-spl:+.3f} dB")


def test_sinc_correction():
    print("\nSinc-Korrektur")
    fs = 622.0
    for f in (50.0, 100.0):
        m_raw = d.Meter(fs, band="10-100", n_fft=1024)
        m_raw.run(d.sine(fs, 8.0, f, 150.0, sensor_average=True))
        # ohne Korrektur muss der Wert genau um sinc_correction_db zu tief liegen
        mags_lvl = m_raw.report()["spl_tone"]
        expected_gap = d.sinc_correction_db(f, fs)
        m_ideal = d.Meter(fs, band="10-100", n_fft=1024)
        m_ideal.run(d.sine(fs, 8.0, f, 150.0, sensor_average=False))
        gap = m_ideal.report()["spl_tone"] - mags_lvl
        check(f"{f:.0f} Hz: Korrektur hebt die Sensormittelung auf",
              close(gap, expected_gap, 0.03),
              f"Differenz {gap:+.3f} dB, erwartet {expected_gap:+.3f} dB")


# -------------------------------------------------------------- Robustheit

def test_drift_rejection():
    print("\nUnterdrueckung von Luftdruckdrift und Offset")
    fs = 622.0
    ref = d.Meter(fs, band="10-100", n_fft=1024)
    ref.run(d.sine(fs, 8.0, 40.0, 150.0, sensor_average=True))
    base = ref.report()["spl_tone"]

    for drift in (20.0, 200.0):  # Pa/s, entspricht starkem Hoehen-/Wetterwechsel
        m = d.Meter(fs, band="10-100", n_fft=1024)
        m.run(d.sine(fs, 8.0, 40.0, 150.0, drift_pa_per_s=drift, sensor_average=True))
        check(f"Drift {drift:.0f} Pa/s aendert den Pegel < 0.05 dB",
              close(m.report()["spl_tone"], base, 0.05),
              f"{m.report()['spl_tone']-base:+.4f} dB")

    for offset in (85000.0, 101325.0, 108000.0):
        m = d.Meter(fs, band="10-100", n_fft=1024)
        m.run(d.sine(fs, 8.0, 40.0, 150.0, offset_pa=offset, sensor_average=True))
        check(f"Luftdruck {offset/100:.0f} hPa aendert den Pegel < 0.02 dB",
              close(m.report()["spl_tone"], base, 0.02),
              f"{m.report()['spl_tone']-base:+.4f} dB")


def test_float32_precision():
    print("\nRechengenauigkeit float32")
    fs = 622.0

    def to_f32(v):
        return struct.unpack("f", struct.pack("f", v))[0]

    ref = d.Meter(fs, band="10-100", n_fft=1024)
    ref.run(d.sine(fs, 8.0, 40.0, 120.0, sensor_average=True))
    base = ref.report()["spl_tone"]

    # Variante A: Rohwert inkl. 101325 Pa Offset nach float32 quantisiert
    m = d.Meter(fs, band="10-100", n_fft=1024)
    m.run(to_f32(p) for p in d.sine(fs, 8.0, 40.0, 120.0, sensor_average=True))
    check("float32 mit vollem Luftdruckoffset genuegt noch bei 120 dB",
          close(m.report()["spl_tone"], base, 0.05),
          f"{m.report()['spl_tone']-base:+.4f} dB")

    lsb = 2.0 ** (math.floor(math.log2(101325.0)) - 23)
    check("float32-Quantisierungsrauschen liegt unter dem Sensorrauschen",
          d.spl_from_pa(lsb / math.sqrt(12.0)) < 60.0,
          f"Schrittweite {lsb:.4f} Pa = {d.spl_from_pa(lsb/math.sqrt(12)):.1f} dB SPL "
          f"gegen ~96 dB Sensorrauschen")


def test_band_switching():
    print("\nBandumschaltung")
    fs = 622.0
    # 130 Hz liegt in 5-150, aber ausserhalb von 10-100.
    # Sollwert = Nennpegel + Filterflanke - Sensormittelung. Der Rohwert des
    # Detektors enthaelt die Sinc-Korrektur noch nicht, deshalb wird sie hier
    # explizit in die Erwartung eingerechnet.
    for band, (lo, hi) in d.BANDS.items():
        m = d.Meter(fs, band=band)
        m.run(d.sine(fs, 8.0, 130.0, 150.0, sensor_average=True))
        lvl = m.slow.spl_rms
        expected = (150.0 + d.BandFilter(lo, hi, fs).response_db(130.0)
                    - d.sinc_correction_db(130.0, fs))
        check(f"Band {band}: 130 Hz trifft den vorhergesagten Pegel",
              close(lvl, expected, 0.15),
              f"{lvl:.2f} dB, erwartet {expected:.2f} dB")
    m = d.Meter(fs, band="10-100")
    m.run(d.sine(fs, 8.0, 130.0, 150.0, sensor_average=True))
    check("Band 10-100 daempft 130 Hz um mehr als 8 dB",
          m.slow.spl_rms < 142.0, f"{m.slow.spl_rms-150.0:+.2f} dB")

    m = d.Meter(fs, band="10-100")
    m.set_band("5-150")
    check("Umschaltung zur Laufzeit setzt die Bandgrenzen",
          m.f_lo == 5.0 and m.f_hi == 150.0, f"{m.f_lo}-{m.f_hi} Hz")
    try:
        d.Meter(fs, band="20-20000")
        check("unbekanntes Band wird abgelehnt", False)
    except ValueError:
        check("unbekanntes Band wird abgelehnt", True)


def test_settling():
    print("\nEinschwingverhalten")
    fs = 622.0
    m = d.Meter(fs, band="10-100")
    amp = d.pa_from_spl(150.0) * math.sqrt(2.0)
    t_filt = None
    for i, p in enumerate(d.sine(fs, 2.0, 40.0, 150.0)):
        x = m.process(p)
        if t_filt is None and abs(x) > 0.99 * amp:
            t_filt = i / fs
    check("Einschwingsperre verhindert, dass der Filterueberschwinger im "
          "Peak-Hold landet",
          d.spl_from_pa(m.fast.peak) < 154.0,
          f"Peak nach Einschwingen {d.spl_from_pa(m.fast.peak):.2f} dB "
          f"gegen 153.01 dB Sollspitze")
    check("Bandfilter erreicht 99 % der Amplitude in < 0.3 s",
          t_filt is not None and t_filt < 0.3, f"{t_filt:.3f} s")

    for label, tau, limit in (("Fast", d.TAU_FAST, 0.30), ("Slow", d.TAU_SLOW, 2.0)):
        m = d.Meter(fs, band="10-100")
        det = m.fast if label == "Fast" else m.slow
        t = None
        for i, p in enumerate(d.sine(fs, 6.0, 40.0, 150.0)):
            m.process(p)
            if t is None and abs(det.spl_rms - 150.0) < 1.0:
                t = i / fs
        check(f"Detektor {label} erreicht +/-1 dB in < {limit} s", t is not None and t < limit,
              f"{t:.3f} s bei tau = {tau} s")


def test_multitone():
    print("\nMehrtonsignal")
    fs = 622.0
    m = d.Meter(fs, band="10-100", n_fft=1024)
    # 35 Hz dominant, 70 Hz 10 dB leiser (typisches Oberwellenbild eines Subs)
    m.run(d.multitone(fs, 8.0, [(35.0, 150.0), (70.0, 140.0)]))
    r = m.report()
    check("dominante Frequenz wird korrekt erkannt", close(r["f0"], 35.0, 0.1),
          f"f0 = {r['f0']:.3f} Hz")
    check("Grundton wird mit seinem eigenen Pegel gemeldet",
          close(r["spl_tone"], 150.0, 0.2), f"{r['spl_tone']:.2f} dB")
    # Summenpegel: 150 dB + 140 dB = 10*log10(10^15 + 10^14) = 150.41 dB
    check("Bandpegel ist die Leistungssumme beider Toene",
          close(r["spl_band_fft"], 150.41, 0.15), f"{r['spl_band_fft']:.2f} dB")


def main():
    print("Abnahmetests DSP-Referenzmodell")
    for fn in (test_butterworth_q, test_band_response, test_tone_accuracy,
               test_interpolation_bias, test_parseval, test_detector_accuracy,
               test_peak_vs_rms, test_linearity, test_sinc_correction,
               test_drift_rejection, test_float32_precision, test_band_switching,
               test_settling, test_multitone):
        fn()
    failed = [n for n, ok, _ in _results if not ok]
    print(f"\n{len(_results) - len(failed)} von {len(_results)} Tests bestanden")
    if failed:
        print("Fehlgeschlagen:")
        for n in failed:
            print(f"  - {n}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
