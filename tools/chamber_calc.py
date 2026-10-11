#!/usr/bin/env python3
"""Akustischer Rechner fuer die Sensorkammer.

Beantwortet die eine Frage, die beim Gehaeuseentwurf zaehlt: wie gross
muessen die Lueftungsoeffnungen sein, damit die Kammer im Messband
(10..150 Hz) nicht stoert?

Hintergrund steht in docs/09-sensorkammer-spezifikation.md.

Aufruf ohne Argumente rechnet den aktuellen Entwurf durch. Eigene Masse:

    python3 tools/chamber_calc.py --volume 110 --area 180 --wall 3.0

Alle Laengen in mm, Flaechen in mm^2, Volumen in cm^3.
"""

from __future__ import annotations

import argparse
import math

# Luft bei 20 C, 1013 hPa
C_AIR = 343.0      # Schallgeschwindigkeit, m/s
RHO = 1.2          # Dichte, kg/m^3
MU = 1.81e-5       # dynamische Viskositaet, Pa*s

F_BAND_MAX = 150.0  # obere Bandgrenze, auf die der Fehler bezogen wird


def helmholtz_hz(volume_m3: float, area_m2: float, wall_m: float,
                 n_holes: int) -> tuple[float, float]:
    """Helmholtz-Frequenz der Kammer. Gibt (f_H, L_eff) zurueck.

    Die Oeffnungen wirken als eine Parallelschaltung: die Gesamtflaeche
    zaehlt, die Muendungskorrektur richtet sich aber nach der EINZELnen
    Oeffnung. Viele kleine Loecher sind deshalb nicht dasselbe wie ein
    grosses gleicher Flaeche -- sie sind besser, weil L_eff kleiner wird.
    """
    a_single = math.sqrt(area_m2 / n_holes / math.pi)
    l_eff = wall_m + 0.85 * a_single        # einseitig geflanscht
    return C_AIR / (2 * math.pi) * math.sqrt(area_m2 / (volume_m3 * l_eff)), l_eff


def response_db(f_hz: float, f_h: float) -> float:
    """Uebertragung Kammerdruck/Aussendruck in dB, ungedaempft gerechnet.

    Unterhalb der Resonanz folgt die Kammer dem Aussendruck mit einer
    leichten UEBERHOEHUNG, nicht mit einer Daempfung. Ohne Daempfungsterm
    ist das Ergebnis die obere Schranke des Fehlers -- real liegt er
    niedriger. Das ist die sichere Richtung.
    """
    ratio2 = (f_hz / f_h) ** 2
    if ratio2 >= 1.0:
        return float("inf")
    return 20 * math.log10(1.0 / (1.0 - ratio2))


def min_area_mm2(volume_m3: float, f_target: float, wall_m: float,
                 n_holes: int) -> float:
    """Kleinste Gesamtflaeche fuer eine gewuenschte Helmholtz-Frequenz.

    Iterativ, weil L_eff selbst von der Flaeche abhaengt. Konvergiert in
    wenigen Schritten; divergiert, wenn das Ziel physikalisch nicht mehr
    erreichbar ist (dann waechst die noetige Flaeche schneller als der
    Gewinn durch sie -- siehe Kommentar zur 1-kHz-Zeile in der Ausgabe).
    """
    a = 0.002
    area = 0.0
    for _ in range(200):
        l_eff = wall_m + 0.85 * a
        area = volume_m3 * l_eff * (2 * math.pi * f_target / C_AIR) ** 2
        a = math.sqrt(area / n_holes / math.pi)
        if a > 0.05:                       # > 100 mm Einzelloch: sinnlos
            return float("inf")
    return area * 1e6


def viscous_pole_hz(volume_m3: float, slot_w_m: float, slot_len_m: float,
                    wall_m: float, n: int) -> float:
    """Grenzfrequenz aus der Zaehigkeit der Luft im Spalt.

    Rechteckspalt: R = 12*mu*L / (b*w^3). Bei allem, was man drucken kann,
    liegt dieser Pol um Groessenordnungen ueber dem Band -- die Funktion
    ist nur da, um das zu belegen statt zu behaupten.
    """
    r_a = 12 * MU * wall_m / (slot_len_m * slot_w_m ** 3) / n
    c_a = volume_m3 / (RHO * C_AIR ** 2)
    return 1.0 / (2 * math.pi * r_a * c_a)


def spl_to_pa_peak(spl_db: float) -> float:
    return 10 ** (spl_db / 20) * 20e-6 * math.sqrt(2)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--volume", type=float, default=110.0,
                    help="Innenvolumen in cm^3 (Kammer + alles, was ueber "
                         "Kabelloecher damit verbunden ist)")
    ap.add_argument("--area", type=float, default=182.0,
                    help="Gesamtflaeche aller Lueftungsoeffnungen in mm^2")
    ap.add_argument("--holes", type=int, default=12,
                    help="Anzahl der Oeffnungen, auf die sich die Flaeche verteilt")
    ap.add_argument("--wall", type=float, default=3.0,
                    help="Wandstaerke an der Oeffnung in mm")
    ap.add_argument("--panel", type=float, nargs=2, default=[100.0, 90.0],
                    metavar=("L", "B"), help="groesste Wandflaeche in mm")
    args = ap.parse_args()

    vol = args.volume * 1e-6
    area = args.area * 1e-6
    wall = args.wall * 1e-3

    print("Sensorkammer, akustische Pruefung")
    print("=" * 62)
    print(f"Volumen           {args.volume:8.1f} cm^3")
    print(f"Oeffnungsflaeche  {args.area:8.1f} mm^2  auf {args.holes} Oeffnungen")
    print(f"Wandstaerke       {args.wall:8.1f} mm")

    f_h, l_eff = helmholtz_hz(vol, area, wall, args.holes)
    err = response_db(F_BAND_MAX, f_h)
    print()
    print(f"Helmholtz-Frequenz        {f_h:8.0f} Hz   (L_eff {l_eff*1e3:.1f} mm)")
    print(f"Fehler bei {F_BAND_MAX:.0f} Hz          {err:+8.2f} dB   "
          f"{'OK' if err < 1.0 else 'ZU HOCH'}")
    print(f"Fehler bei  10 Hz         {response_db(10, f_h):+8.2f} dB")

    f_visc = viscous_pole_hz(vol, 0.002, 0.008, wall, args.holes)
    print(f"Viskoser Pol              {f_visc:8.3g} Hz   "
          "(unkritisch, solange >> 150 Hz)")

    print()
    print("Mindestflaeche fuer verschiedene Ziele:")
    for f_t in (300, 500, 700, 1000):
        s = min_area_mm2(vol, f_t, wall, args.holes)
        if math.isinf(s):
            print(f"  f_H >= {f_t:4d} Hz   nicht erreichbar bei diesem Volumen")
        else:
            e = response_db(F_BAND_MAX, f_t)
            print(f"  f_H >= {f_t:4d} Hz   S >= {s:7.1f} mm^2   "
                  f"Fehler {e:+.2f} dB")
    print("  Die Flaeche waechst schneller als quadratisch: groessere Loecher")
    print("  haben auch mehr Muendungskorrektur. Ueber etwa 1 kHz lohnt sich")
    print("  das nicht mehr -- dann lieber das Volumen verkleinern.")

    print()
    print("Stehende Wellen: unkritisch, solange die laengste Innenkante")
    print(f"  unter lambda/10 = {C_AIR / F_BAND_MAX / 10 * 1e3:.0f} mm bleibt.")

    print()
    print("Kraft auf die groesste Wand (fuer die Steifigkeit):")
    panel_m2 = args.panel[0] * args.panel[1] * 1e-6
    for spl in (150, 155):
        pa = spl_to_pa_peak(spl)
        print(f"  {spl} dB -> {pa:6.0f} Pa Spitze -> {pa * panel_m2:5.1f} N")
    print("  Unkritisch fuer die Messung, solange gut belueftet: Luft stroemt")
    print("  schneller nach, als die Wand sich biegt. Zaehlt nur mechanisch.")


if __name__ == "__main__":
    main()
