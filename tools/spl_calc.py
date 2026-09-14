#!/usr/bin/env python3
"""Auslegungsrechner fuer das Bass-SPL-Meter.

Rechnet zwischen Schalldruckpegel (dB SPL, Bezug 20 uPa) und Druck in Pa um und
zeigt, welche Sensor-Aufloesung, welcher Messbereich und welche FFT-Parameter
fuer den Zielbereich 10-100 Hz / 120-180 dB noetig sind.

Aufruf:  python3 tools/spl_calc.py
"""

import math

P_REF = 20e-6  # Pa, Bezugsschalldruck


def spl_to_pa(spl_db: float) -> float:
    """dB SPL -> Effektivdruck in Pa."""
    return P_REF * 10 ** (spl_db / 20)


def pa_to_spl(pa_rms: float) -> float:
    """Effektivdruck in Pa -> dB SPL."""
    return 20 * math.log10(pa_rms / P_REF)


def sinc_correction_db(f_hz: float, fs_hz: float) -> float:
    """Amplitudenfehler durch die Mittelung des Sensor-ADC ueber ein Abtastintervall.

    Die Wandlung integriert ueber 1/fs, das ergibt einen sinc-Tiefpass.
    Rueckgabe: Korrektur in dB, die auf den Messwert addiert werden muss.
    """
    x = math.pi * f_hz / fs_hz
    if x == 0:
        return 0.0
    return -20 * math.log10(math.sin(x) / x)


def fft_params(fs_hz: float, n: int, hop_ratio: float = 0.25) -> dict:
    return {
        "fs_hz": fs_hz,
        "n": n,
        "bin_hz": fs_hz / n,
        "window_s": n / fs_hz,
        "update_s": n * hop_ratio / fs_hz,
        "nyquist_hz": fs_hz / 2,
    }


def main() -> None:
    print("== Pegel -> Druck ==")
    print(f"{'dB SPL':>8} {'p_rms [Pa]':>12} {'p_peak [Pa]':>12} {'p_peak [hPa]':>13}")
    for spl in (90, 100, 110, 120, 130, 140, 150, 155, 160, 170, 180):
        p = spl_to_pa(spl)
        print(f"{spl:>8} {p:>12.3f} {p*math.sqrt(2):>12.1f} {p*math.sqrt(2)/100:>13.2f}")

    print("\n== Sensor-Check BMP581 (30-125 kPa, LSB 1/64 Pa) ==")
    lsb = 1 / 64
    print(f"LSB {lsb:.5f} Pa entspricht {pa_to_spl(lsb):.1f} dB SPL (theoretische Untergrenze)")
    for noise_pa, label in ((0.08, "hohes Oversampling"), (1.3, "OSR 1x, ~600 Hz ODR")):
        print(f"Rauschen {noise_pa:>5.2f} Pa ({label:22}) -> Rauschteppich {pa_to_spl(noise_pa):5.1f} dB SPL")
    p_amb_hpa = 1013.0
    p_max_hpa, p_min_hpa = 1250.0, 300.0  # Sensorbereich BMP581
    headroom_pa = min(p_max_hpa - p_amb_hpa, p_amb_hpa - p_min_hpa) * 100
    print(f"Umgebungsdruck {p_amb_hpa:.0f} hPa, Sensorbereich {p_min_hpa:.0f}-{p_max_hpa:.0f} hPa")
    print(f"  -> Aussteuerreserve {headroom_pa/100:.0f} hPa Spitze = "
          f"{pa_to_spl(headroom_pa / math.sqrt(2)):.1f} dB SPL Clipping-Grenze (Sinus)")

    print("\n== Frequenzgang-Korrektur (sinc) ==")
    for fs in (500.0, 622.0):
        vals = ", ".join(f"{f} Hz: +{sinc_correction_db(f, fs):.2f} dB" for f in (10, 50, 100, 150))
        print(f"fs = {fs:.0f} Hz -> {vals}")

    print("\n== FFT-Parameter ==")
    for n in (256, 512, 1024, 2048):
        p = fft_params(500.0, n)
        print(f"N={n:>5}: Aufloesung {p['bin_hz']:.3f} Hz, Fenster {p['window_s']:.2f} s, "
              f"Update {p['update_s']:.2f} s")

    print("\n== Statische Kalibrierung per Wassersaeule ==")
    for cm in (5, 10, 20, 50):
        pa = 999.0 * 9.81 * cm / 100  # Wasser bei 15 C
        print(f"{cm:>3} cm H2O = {pa:7.1f} Pa = {pa_to_spl(pa):.1f} dB SPL (statisch, als Gain-Test)")


if __name__ == "__main__":
    main()
