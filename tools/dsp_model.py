#!/usr/bin/env python3
"""Referenzmodell der kompletten Messkette (Phase 1).

Bewusst ohne numpy/scipy und strikt sample-by-sample geschrieben: dieser Code
ist die Vorlage fuer die C++-Portierung auf den ESP32. Jede Funktion hier hat
ein direktes Gegenstueck in firmware/src/.

Kette:
    Rohdruck [Pa]
      -> DC-Blocker (Hochpass 1. Ordnung, 0.5 Hz)   entfernt Luftdruck + Drift
      -> Bandfilter (Butterworth 4. Ordnung HP + LP) definiert das Messband
      -> Pegeldetektoren: exponentieller RMS (Fast/Slow) + True-Peak
      -> parallel: Hann-FFT -> Spektrum, dominante Frequenz f0

Aufruf:  python3 tools/dsp_model.py     (Demo + Kennwerttabellen)
Tests:   python3 tools/test_dsp_model.py
"""

import cmath
import math

P_REF = 20e-6  # Pa, Bezugsschalldruck fuer 0 dB SPL

# Messbaender, im Geraet per Menue umschaltbar.
# Die Grenzen sind die -3-dB-Punkte des Bandfilters, wie bei Messfiltern ueblich.
BANDS = {
    "10-100": (10.0, 100.0),
    "5-150": (5.0, 150.0),
}

# Zeitkonstanten der Pegeldetektoren (wie bei Schallpegelmessern)
TAU_FAST = 0.125  # s
TAU_SLOW = 1.0    # s

# Sperrzeit fuer die Haltewerte nach Start, Reset oder Bandwechsel. Ein
# Butterworth-Bandfilter ueberschwingt beim Einschwingen um gut 1 dB; ohne
# Sperre landet dieser Ueberschwinger als vermeintlicher Spitzenpegel im
# Peak-Hold und das Geraet zeigt dauerhaft einen zu hohen Wert an.
HOLD_GUARD = 0.15  # s


def spl_from_pa(p_rms: float) -> float:
    """Effektivdruck [Pa] -> dB SPL. Gibt -inf bei 0."""
    if p_rms <= 0.0:
        return float("-inf")
    return 20.0 * math.log10(p_rms / P_REF)


def pa_from_spl(spl_db: float) -> float:
    """dB SPL -> Effektivdruck [Pa]."""
    return P_REF * 10.0 ** (spl_db / 20.0)


# ---------------------------------------------------------------- Biquads

class Biquad:
    """Biquad in Transposed Direct Form II (numerisch guenstig, 2 Zustaende)."""

    def __init__(self, b0: float, b1: float, b2: float, a1: float, a2: float):
        self.b0, self.b1, self.b2, self.a1, self.a2 = b0, b1, b2, a1, a2
        self.z1 = 0.0
        self.z2 = 0.0

    def reset(self) -> None:
        self.z1 = self.z2 = 0.0

    def process(self, x: float) -> float:
        y = self.b0 * x + self.z1
        self.z1 = self.b1 * x - self.a1 * y + self.z2
        self.z2 = self.b2 * x - self.a2 * y
        return y

    def response(self, f: float, fs: float) -> complex:
        """Komplexer Frequenzgang bei f."""
        z = cmath.exp(-2j * math.pi * f / fs)
        return ((self.b0 + self.b1 * z + self.b2 * z * z) /
                (1.0 + self.a1 * z + self.a2 * z * z))


def butterworth_q(order: int) -> list:
    """Guetefaktoren der Biquad-Sektionen eines Butterworth-Filters.

    Ordnung 4 -> [0.5412, 1.3066]. Nur gerade Ordnungen.
    """
    if order % 2 != 0 or order < 2:
        raise ValueError("nur gerade Ordnungen >= 2")
    return [1.0 / (2.0 * math.cos((2 * k + 1) * math.pi / (2 * order)))
            for k in range(order // 2)]


def lowpass_biquad(fc: float, fs: float, q: float) -> Biquad:
    """RBJ-Cookbook-Tiefpass, 2. Ordnung."""
    w0 = 2.0 * math.pi * fc / fs
    cw, sw = math.cos(w0), math.sin(w0)
    alpha = sw / (2.0 * q)
    a0 = 1.0 + alpha
    return Biquad(((1.0 - cw) / 2.0) / a0, (1.0 - cw) / a0, ((1.0 - cw) / 2.0) / a0,
                  (-2.0 * cw) / a0, (1.0 - alpha) / a0)


def highpass_biquad(fc: float, fs: float, q: float) -> Biquad:
    """RBJ-Cookbook-Hochpass, 2. Ordnung."""
    w0 = 2.0 * math.pi * fc / fs
    cw, sw = math.cos(w0), math.sin(w0)
    alpha = sw / (2.0 * q)
    a0 = 1.0 + alpha
    return Biquad(((1.0 + cw) / 2.0) / a0, (-(1.0 + cw)) / a0, ((1.0 + cw) / 2.0) / a0,
                  (-2.0 * cw) / a0, (1.0 - alpha) / a0)


class DcBlocker:
    """Hochpass 1. Ordnung. Entfernt den statischen Luftdruck und langsame Drift.

    y[n] = a * (y[n-1] + x[n] - x[n-1]),  a = exp(-2*pi*fc/fs)
    """

    def __init__(self, fc: float, fs: float):
        self.a = math.exp(-2.0 * math.pi * fc / fs)
        self.x1 = 0.0
        self.y1 = 0.0
        self.primed = False

    def reset(self) -> None:
        self.x1 = self.y1 = 0.0
        self.primed = False

    def process(self, x: float) -> float:
        if not self.primed:
            # Erster Wert definiert den Arbeitspunkt, sonst schwingt der Filter
            # nach dem Einschalten ueber 100 kPa aus.
            self.x1 = x
            self.primed = True
        y = self.a * (self.y1 + x - self.x1)
        self.x1 = x
        self.y1 = y
        return y

    def response(self, f: float, fs: float) -> complex:
        z = cmath.exp(-2j * math.pi * f / fs)
        return self.a * (1.0 - z) / (1.0 - self.a * z)


class BandFilter:
    """Messbandfilter: Butterworth-Hochpass + -Tiefpass, je 4. Ordnung.

    -3 dB an den Bandgrenzen, 24 dB/Oktave Flankensteilheit.
    """

    def __init__(self, f_lo: float, f_hi: float, fs: float, order: int = 4):
        if f_hi >= fs / 2.0:
            raise ValueError(f"obere Bandgrenze {f_hi} Hz >= Nyquist {fs/2} Hz")
        self.f_lo, self.f_hi, self.fs, self.order = f_lo, f_hi, fs, order
        qs = butterworth_q(order)
        self.sections = ([highpass_biquad(f_lo, fs, q) for q in qs] +
                         [lowpass_biquad(f_hi, fs, q) for q in qs])

    def reset(self) -> None:
        for s in self.sections:
            s.reset()

    def process(self, x: float) -> float:
        for s in self.sections:
            x = s.process(x)
        return x

    def response_db(self, f: float) -> float:
        h = 1.0 + 0j
        for s in self.sections:
            h *= s.response(f, self.fs)
        mag = abs(h)
        return 20.0 * math.log10(mag) if mag > 0 else float("-inf")


def sinc_correction_db(f: float, fs: float) -> float:
    """Korrektur des Sensor-Tiefpasses (ADC mittelt ueber ein Abtastintervall)."""
    x = math.pi * f / fs
    if x == 0.0:
        return 0.0
    return -20.0 * math.log10(math.sin(x) / x)


# ---------------------------------------------------------------- Detektoren

class LevelDetector:
    """Exponentieller RMS-Detektor plus True-Peak-Halteglied."""

    def __init__(self, fs: float, tau: float):
        self.alpha = 1.0 - math.exp(-1.0 / (fs * tau))
        self.ms = 0.0        # gleitender quadratischer Mittelwert
        self.peak = 0.0      # groesster Betrag seit dem letzten Reset
        self.max_rms = 0.0   # groesster RMS-Wert seit dem letzten Reset

    def reset_hold(self) -> None:
        self.peak = 0.0
        self.max_rms = 0.0

    def process(self, x: float, hold: bool = True) -> float:
        """hold=False friert die Haltewerte ein (Einschwingsperre)."""
        self.ms += self.alpha * (x * x - self.ms)
        rms = math.sqrt(self.ms)
        if hold:
            if rms > self.max_rms:
                self.max_rms = rms
            a = abs(x)
            if a > self.peak:
                self.peak = a
        return rms

    @property
    def spl_rms(self) -> float:
        return spl_from_pa(math.sqrt(self.ms))

    @property
    def spl_max_rms(self) -> float:
        return spl_from_pa(self.max_rms)

    @property
    def spl_peak(self) -> float:
        """True-Peak als Pegel. Bezug bleibt 20 uPa, also kein /sqrt(2)."""
        return spl_from_pa(self.peak)


# ---------------------------------------------------------------- Spektrum

def _fft(x: list) -> list:
    """Iterative Radix-2-FFT. Laenge muss Zweierpotenz sein."""
    n = len(x)
    if n & (n - 1):
        raise ValueError("FFT-Laenge muss Zweierpotenz sein")
    a = list(x)
    # Bit-Reversal-Permutation
    j = 0
    for i in range(1, n):
        bit = n >> 1
        while j & bit:
            j ^= bit
            bit >>= 1
        j |= bit
        if i < j:
            a[i], a[j] = a[j], a[i]
    length = 2
    while length <= n:
        ang = -2.0 * math.pi / length
        wl = cmath.exp(1j * ang)
        for i in range(0, n, length):
            w = 1.0 + 0j
            half = length >> 1
            for k in range(half):
                u = a[i + k]
                v = a[i + k + half] * w
                a[i + k] = u + v
                a[i + k + half] = u - v
                w *= wl
        length <<= 1
    return a


def hann_window(n: int) -> list:
    """Periodisches Hann-Fenster (fuer Spektralanalyse korrekt, nicht symmetrisch)."""
    return [0.5 - 0.5 * math.cos(2.0 * math.pi * i / n) for i in range(n)]


class Spectrum:
    """Hann-gefensterte FFT mit Peak-Interpolation und Bandpegelberechnung."""

    # Leistungskorrektur des Hann-Fensters: mean(w^2) = 3/8
    WINDOW_POWER = 0.375

    def __init__(self, n: int, fs: float):
        self.n = n
        self.fs = fs
        self.window = hann_window(n)
        self.bin_hz = fs / n

    def magnitudes(self, block: list) -> list:
        """Einseitiges Betragsspektrum in Pa (roh, ohne Fensterskalierung)."""
        if len(block) != self.n:
            raise ValueError(f"Block muss {self.n} Werte haben, hat {len(block)}")
        spec = _fft([complex(block[i] * self.window[i], 0.0) for i in range(self.n)])
        return [abs(spec[k]) for k in range(self.n // 2 + 1)]

    def band_rms(self, mags: list, f_lo: float, f_hi: float,
                 sinc_correct: bool = True) -> float:
        """Effektivdruck [Pa] im Band, ueber Parseval aus dem Spektrum.

        Rechteckige Bandgrenzen: exakter als ein Zeitbereichsfilter, weil keine
        Flankenform hineinspielt. Dient als Gegenprobe zum Detektorpfad.
        """
        k_lo = max(1, int(math.ceil(f_lo / self.bin_hz)))
        k_hi = min(len(mags) - 1, int(math.floor(f_hi / self.bin_hz)))
        acc = 0.0
        for k in range(k_lo, k_hi + 1):
            m = mags[k]
            if sinc_correct:
                m *= 10.0 ** (sinc_correction_db(k * self.bin_hz, self.fs) / 20.0)
            # Faktor 2 fuer die gespiegelte negative Frequenzhaelfte
            acc += 2.0 * m * m
        return math.sqrt(acc / (self.n * self.n * self.WINDOW_POWER))

    def dominant(self, mags: list, f_lo: float, f_hi: float) -> tuple:
        """(f0 [Hz], Effektivdruck des Tons [Pa]) des staerksten Anteils im Band.

        f0 per parabolischer Interpolation ueber die logarithmierten Betraege der
        drei Bins um das Maximum. Die Amplitude wird ueber die Hauptkeule des
        Hann-Fensters (Peak +/- 2 Bins) summiert, damit Scalloping-Verluste
        zwischen zwei Bins nicht durchschlagen.
        """
        # Suchbereich um ein Bin ueber die Bandgrenzen hinaus erweitern: ein Ton
        # genau auf der Bandgrenze hat sein Maximum sonst evtl. im ersten Bin
        # ausserhalb, die Interpolation liefe in die Begrenzung.
        k_lo = max(1, int(math.ceil(f_lo / self.bin_hz)) - 1)
        k_hi = min(len(mags) - 2, int(math.floor(f_hi / self.bin_hz)) + 1)
        if k_hi <= k_lo:
            return (0.0, 0.0)
        k0 = max(range(k_lo, k_hi + 1), key=lambda k: mags[k])

        delta = 0.0
        if 1 <= k0 < len(mags) - 1:
            eps = 1e-30
            a = math.log(mags[k0 - 1] + eps)
            b = math.log(mags[k0] + eps)
            c = math.log(mags[k0 + 1] + eps)
            denom = a - 2.0 * b + c
            if denom != 0.0:
                delta = 0.5 * (a - c) / denom
                delta = max(-0.5, min(0.5, delta))
        f0 = (k0 + delta) * self.bin_hz

        acc = 0.0
        for k in range(max(1, k0 - 2), min(len(mags) - 1, k0 + 2) + 1):
            acc += 2.0 * mags[k] * mags[k]
        p_rms = math.sqrt(acc / (self.n * self.n * self.WINDOW_POWER))
        p_rms *= 10.0 ** (sinc_correction_db(f0, self.fs) / 20.0)
        return (f0, p_rms)


# ---------------------------------------------------------------- Gesamtkette

class Meter:
    """Komplette Messkette, so wie sie auf dem ESP32 laufen soll."""

    def __init__(self, fs: float, band: str = "10-100", n_fft: int = 512,
                 dc_fc: float = 0.5):
        if band not in BANDS:
            raise ValueError(f"unbekanntes Band {band!r}, erlaubt: {list(BANDS)}")
        f_lo, f_hi = BANDS[band]
        self.fs = fs
        self.band = band
        self.f_lo, self.f_hi = f_lo, f_hi
        self.dc = DcBlocker(dc_fc, fs)
        self.filt = BandFilter(f_lo, f_hi, fs)
        self.fast = LevelDetector(fs, TAU_FAST)
        self.slow = LevelDetector(fs, TAU_SLOW)
        self.spectrum = Spectrum(n_fft, fs)
        self.fft_buf = []
        self._guard = int(fs * HOLD_GUARD)
        self.last_f0 = 0.0
        self.last_tone_pa = 0.0
        self.last_band_pa = 0.0

    def set_band(self, band: str) -> None:
        """Bandumschaltung zur Laufzeit. Filter wird neu ausgelegt und geleert."""
        f_lo, f_hi = BANDS[band]
        self.band = band
        self.f_lo, self.f_hi = f_lo, f_hi
        self.filt = BandFilter(f_lo, f_hi, self.fs)
        self.reset_hold()

    def process(self, p_raw: float) -> float:
        """Ein Rohsample [Pa] verarbeiten, gibt den bandgefilterten Wert zurueck.

        Wichtig: die FFT bekommt das Signal VOR dem Bandfilter (nur DC-befreit).
        Sonst erbt die Frequenzanzeige die -3 dB der Filterflanke und ein Ton
        genau auf einer Bandgrenze wuerde 3 dB zu niedrig angezeigt. Die
        Bandgrenzen wirken im Spektralpfad stattdessen rechteckig ueber die
        Bin-Auswahl.
        """
        d = self.dc.process(p_raw)
        x = self.filt.process(d)
        hold = self._guard <= 0
        if not hold:
            self._guard -= 1
        self.fast.process(x, hold)
        self.slow.process(x, hold)
        self.fft_buf.append(d)
        if len(self.fft_buf) >= self.spectrum.n:
            block = self.fft_buf[-self.spectrum.n:]
            del self.fft_buf[:self.spectrum.n // 4]  # Hop = N/4
            mags = self.spectrum.magnitudes(block)
            self.last_f0, self.last_tone_pa = self.spectrum.dominant(
                mags, self.f_lo, self.f_hi)
            self.last_band_pa = self.spectrum.band_rms(mags, self.f_lo, self.f_hi)
        return x

    def run(self, samples) -> None:
        for p in samples:
            self.process(p)

    def reset_hold(self) -> None:
        """Haltewerte loeschen und die Einschwingsperre neu starten."""
        self.fast.reset_hold()
        self.slow.reset_hold()
        self._guard = int(self.fs * HOLD_GUARD)

    def sinc_offset_db(self) -> float:
        """Sinc-Korrektur fuer den Zeitbereichspfad.

        Im Zeitbereich laesst sich die Korrektur nicht pro Frequenz anwenden,
        deshalb wird sie mit der gerade dominanten Frequenz f0 gebildet. Bei
        Bassmessungen dominiert praktisch immer ein Ton, der Ansatz stimmt dann.
        Bei breitbandigem Inhalt bleibt ein Restfehler, der aber durch die
        Korrekturgroesse selbst begrenzt ist: unter 0.4 dB bis 100 Hz.
        """
        return sinc_correction_db(self.last_f0, self.fs) if self.last_f0 > 0 else 0.0

    def report(self) -> dict:
        corr = self.sinc_offset_db()
        return {
            "band": self.band,
            "spl_fast": self.fast.spl_rms + corr,
            "spl_slow": self.slow.spl_rms + corr,
            "spl_max_rms": self.fast.spl_max_rms + corr,
            "spl_peak": self.fast.spl_peak + corr,
            "sinc_corr": corr,
            "f0": self.last_f0,
            "spl_tone": spl_from_pa(self.last_tone_pa),
            "spl_band_fft": spl_from_pa(self.last_band_pa),
        }


# ---------------------------------------------------------------- Testsignale

def sine(fs: float, seconds: float, freq: float, spl_db: float,
         offset_pa: float = 101325.0, drift_pa_per_s: float = 0.0,
         phase: float = 0.0, sensor_average: bool = False):
    """Sinus mit gegebenem Pegel auf statischem Luftdruck, optional mit Drift.

    sensor_average=True bildet nach, was der Sensor-ADC tatsaechlich tut: er
    mittelt ueber ein Abtastintervall statt momentan abzutasten. Analytisch ist
    der Mittelwert von A*sin(wt+p) ueber [t, t+T] gleich
    A*sinc(f*T)*sin(w*(t+T/2)+p) -- also Amplitude mal sinc und eine halbe
    Abtastperiode Verzoegerung. Genau diesen Verlust hebt sinc_correction_db()
    spaeter wieder auf; mit diesem Schalter laesst sich die Korrektur pruefen.
    """
    amp = pa_from_spl(spl_db) * math.sqrt(2.0)
    t_shift = 0.0
    if sensor_average:
        x = math.pi * freq / fs
        amp *= math.sin(x) / x if x != 0.0 else 1.0
        t_shift = 0.5 / fs
    n = int(round(fs * seconds))
    for i in range(n):
        t = i / fs
        yield (offset_pa + drift_pa_per_s * t +
               amp * math.sin(2.0 * math.pi * freq * (t + t_shift) + phase))


def multitone(fs: float, seconds: float, components, offset_pa: float = 101325.0):
    """components: Liste von (freq, spl_db)."""
    parts = [(2.0 * math.pi * f, pa_from_spl(l) * math.sqrt(2.0)) for f, l in components]
    n = int(round(fs * seconds))
    for i in range(n):
        t = i / fs
        yield offset_pa + sum(a * math.sin(w * t) for w, a in parts)


# ---------------------------------------------------------------- Demo

def _print_band_response(fs: float) -> None:
    print(f"\n== Bandfiltergang bei fs = {fs:.0f} Hz ==")
    freqs = [5, 7, 10, 15, 20, 30, 50, 80, 100, 120, 150, 200]
    print(f"{'f [Hz]':>8}" + "".join(f"{name+' [dB]':>14}" for name in BANDS))
    for f in freqs:
        row = f"{f:>8}"
        for name, (lo, hi) in BANDS.items():
            bf = BandFilter(lo, hi, fs)
            row += f"{bf.response_db(f):>14.2f}"
        print(row)


def _print_chain_check(fs: float) -> None:
    print(f"\n== Gesamtkette, Sinus 150.0 dB, fs = {fs:.0f} Hz, Band 10-100 ==")
    print("Testsignal mit nachgebildeter Sensormittelung (sensor_average=True)")
    print("Detektorpfad = Zeitbereich hinter dem Bandfilter (-3 dB an den Grenzen)")
    print("Tonpfad      = FFT vor dem Bandfilter, beide sinc-korrigiert")
    print(f"{'f [Hz]':>8} {'Detektor':>10} {'Fehler':>8} {'Ton FFT':>10} {'Fehler':>8} "
          f"{'f0 [Hz]':>10} {'Fehler':>9}")
    for f in (10.0, 20.0, 31.5, 50.0, 63.0, 80.0, 100.0):
        m = Meter(fs, band="10-100", n_fft=1024)
        m.run(sine(fs, 10.0, f, 150.0, sensor_average=True))
        r = m.report()
        print(f"{f:>8.1f} {r['spl_slow']:>10.2f} {r['spl_slow']-150.0:>+8.2f} "
              f"{r['spl_tone']:>10.2f} {r['spl_tone']-150.0:>+8.2f} "
              f"{r['f0']:>10.3f} {r['f0']-f:>+9.3f}")


def main() -> None:
    for fs in (500.0, 622.0):
        _print_band_response(fs)
    _print_chain_check(622.0)

    print("\n== Einschwingzeit (Sprung auf 150 dB @ 40 Hz) ==")
    fs = 622.0
    for band in BANDS:
        print(f"  Band {band}:")
        for label, tau in (("Fast", TAU_FAST), ("Slow", TAU_SLOW)):
            m = Meter(fs, band=band)
            det = m.fast if label == "Fast" else m.slow
            t_settle = None
            for i, p in enumerate(sine(fs, 6.0, 40.0, 150.0)):
                m.process(p)
                if t_settle is None and abs(det.spl_rms - 150.0) < 1.0:
                    t_settle = i / fs
            print(f"    Detektor {label} (tau = {tau:5.3f} s): 150 dB +/-1 dB nach "
                  f"{t_settle:.3f} s")
        # Reines Filtereinschwingen, ohne Detektortraegheit: wann erreicht die
        # Huellkurve des gefilterten Signals 99 % des Endwerts?
        m = Meter(fs, band=band)
        amp_target = pa_from_spl(150.0) * math.sqrt(2.0)
        t_filt = None
        for i, p in enumerate(sine(fs, 2.0, 40.0, 150.0)):
            x = m.process(p)
            if t_filt is None and abs(x) > 0.99 * amp_target:
                t_filt = i / fs
        print(f"    Bandfilter allein: 99 % der Amplitude nach {t_filt:.3f} s")


if __name__ == "__main__":
    main()
