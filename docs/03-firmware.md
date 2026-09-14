# 3. Firmware

Toolchain: **PlatformIO + Arduino-Framework für ESP32**. Bibliotheken:
`SparkFun_BMP581_Arduino_Library`, `TFT_eSPI`, `ESP-DSP` (FFT), `SD`.

> Die komplette Kette ist in Phase 1 als Referenzmodell gebaut und abgesichert:
> `tools/dsp_model.py`, 99 Abnahmetests in `tools/test_dsp_model.py`.
> Messergebnisse und drei daraus folgende Konstruktionsentscheidungen stehen in
> [06-phase1-ergebnisse.md](06-phase1-ergebnisse.md).

## Signalkette

```
BMP581 FIFO (fs ~500-622 Hz, OSR 1x, IIR aus)
   |  Burst-Read 32 Werte alle ~40 ms
   v
Ringpuffer  [float Pa]
   |
   +-- DC-Blocker: Hochpass 1. Ordnung @ 0,5 Hz   (Luftdruck, Wetterdrift)
   |
   +---------------------------+
   |                           |
   v                           v
Bandfilter                  Hann-FFT (N=512/1024, Hop N/4)
Butterworth 4. Ord.         Peak-Bin + parabolische Interpolation
HP + LP, umschaltbar        + Sinc-Korrektur pro Bin
10-100 Hz / 5-150 Hz           |
   |                           +-> f0 (dominante Frequenz)
   +-> RMS Fast (125 ms)       +-> Tonpegel (Hauptkeule, Peak +/- 2 Bins)
   +-> RMS Slow (1 s)          +-> Bandpegel (Parseval, rechteckige Grenzen)
   +-> True-Peak               +-> Spektrum fuer die RTA-Anzeige
   +-> Max-Hold
   |
   +-- Sinc-Korrektur ueber f0
```

**Die FFT hängt bewusst vor dem Bandfilter.** Läge sie dahinter, erbte die
Frequenz- und Tonpegelanzeige die −3 dB der Filterflanke, und ein Ton genau auf
einer Bandgrenze würde 3 dB zu niedrig angezeigt. Die Bandgrenzen wirken im
Spektralpfad stattdessen rechteckig über die Bin-Auswahl. Die Peak-Suche läuft
dabei ein Bin über die Bandgrenzen hinaus, sonst läuft die Interpolation bei
einem Ton exakt auf der Grenze in ihre Begrenzung.

## Messbänder

Im Menü umschaltbar. Die Grenzen sind die −3-dB-Punkte des Bandfilters, wie bei
Messfiltern üblich.

| Band | Zweck |
|---|---|
| **10-100 Hz** | Vergleichbarkeit mit kommerziellen Bass-Metern |
| **5-150 Hz** | erfasst Infraschall unter 10 Hz und unteren Kickbass, liest je nach Musik 1-3 dB höher |

Das aktive Band steht immer in der Statuszeile. Ein Bandwechsel muss die
Haltewerte zurücksetzen (siehe Einschwingsperre).

## Koeffizienten zur Laufzeit berechnen

Keine festen Koeffiziententabellen im Code. Der Sensor taktet mit einem
internen RC-Oszillator, die reale Abtastrate weicht bis zu ±5 % vom Nennwert
ab — eine Tabelle für „622 Hz" wäre um genau diesen Fehler daneben, sowohl in
den Filtergrenzen als auch in der FFT-Frequenzachse. Die Firmware legt die acht
Biquads beim Start aus der gemessenen Abtastrate aus (zwei `sin`/`cos` pro
Sektion, vernachlässigbar).

## Einschwingsperre für die Haltewerte

Ein Butterworth-Bandfilter überschwingt beim Einschwingen um gut 1 dB. Ohne
Gegenmaßnahme landet dieser Überschwinger als vermeintlicher Spitzenpegel im
Peak-Hold — im Test **+4,10 dB statt +3,01 dB** über RMS. Nach Start,
Peak-Reset und Bandwechsel werden die Haltewerte deshalb 150 ms lang
eingefroren.

## Abtasttakt und Frequenzgenauigkeit

Der Sensor taktet sich selbst; die reale ODR weicht vom Nennwert ab (interner
RC-Oszillator, ±5 %). Das geht **1:1 in die Frequenzanzeige ein** — 5 % Fehler
bei 40 Hz sind 2 Hz, das ist beim Anlagentuning zu viel.

Lösung: beim Start einmalig kalibrieren.

```
fs_real = (Anzahl FIFO-Samples in T) / T,  T >= 10 s, gemessen mit esp_timer
```

Der Wert wird in NVS gespeichert und für Bandpass, Sinc-Korrektur und
FFT-Frequenzachse verwendet. Erneute Messung bei jedem Kaltstart (temperatur-
abhängig).

## FFT-Parameter

Basis fs = 500 Hz:

| N | Auflösung | Fensterlänge | Update | Einsatz |
|---|---|---|---|---|
| 256 | 1,95 Hz | 0,51 s | 0,13 s | Live-Balken, schnelle Reaktion |
| **512** | **0,98 Hz** | **1,02 s** | **0,26 s** | Standard |
| 1024 | 0,49 Hz | 2,05 s | 0,51 s | Tuning-Modus, feine f0-Bestimmung |

Mit parabolischer Interpolation über die Hann-Peakspitze ist f0 auch bei N=512
auf besser als 0,1 Hz bestimmbar, solange ein dominanter Ton anliegt.

## Betriebsmodi

| Modus | Anzeige | Detektor |
|---|---|---|
| **LIVE** | große dB-Zahl + f0 + Balkenspektrum | RMS Fast (125 ms) |
| **PEAK / BURP** | max. RMS groß, True-Peak als Nebenwert, f0 beim Maximum | beide, Reset per Taster |
| **RTA** | Spektrum 10-100 Hz, 1/6-Oktav-Balken, Max-Hold | N=1024 |
| **LOG** | läuft im Hintergrund | CSV auf microSD: `t,SPL_rms,SPL_peak,f0,band` @ 4 Hz |

Der PEAK-Modus zeigt **beide** Maxima: den größten RMS-Wert (125 ms) groß, den
True-Peak kleiner daneben. Bei einem Sinus liegen sie exakt 3,01 dB
auseinander; bei echter Musik zeigt der Abstand, wie impulshaltig das Signal
ist. Welcher Wert zitiert wird, entscheidet so der Anwender und nicht die
Firmware.

Umschaltung per Taster/Touch. Peak-Reset: langer Druck.

Anzeigewerte immer mit Bezug kennzeichnen: `dB SPL (Z, 10-100 Hz)`. Keine
A-Bewertung — die würde bei 30 Hz über 39 dB abziehen und ist für Bassmessung
sinnlos. Für Vergleichbarkeit mit Wettbewerbsmessungen zusätzlich den Peak-Wert
ausgeben, das ist dort der übliche Bezug.

## Zeitbudget (ESP32 @ 240 MHz)

| Task | Kern | Last |
|---|---|---|
| FIFO-Read + Filter | 0 | < 1 % |
| FFT 512 (ESP-DSP) | 0 | ~2 % bei 4 Hz Updaterate |
| Display-Refresh | 1 | 10-20 % |
| SD-Logging | 1 | < 5 % |

Reichlich Reserve. Sensor-Task mit hoher Priorität und eigenem Kern, damit der
Display-Refresh keine FIFO-Overruns verursacht (FIFO fasst nur 32 Werte ≈ 55 ms
bei 580 Hz — deshalb mindestens alle 40 ms auslesen).

## Plausibilitätsprüfungen in der Firmware

- **FIFO-Overrun-Flag** auswerten → Messung als ungültig markieren, nicht still
  weiterrechnen.
- **Clipping-Erkennung**: Rohdruck außerhalb 350-1200 hPa → „OVER" anzeigen.
- **Temperatur** mitloggen; der BMP581 liefert sie ohnehin.
- **Sanity-Check beim Start**: Absolutdruck muss 800-1100 hPa sein, sonst
  Sensorfehler.

## Struktur des Repos (geplant)

```
firmware/
  platformio.ini
  src/
    main.cpp            Task-Setup, Modus-Statemachine
    sensor_bmp581.cpp   Init, Continuous Mode, FIFO-Burst, fs-Kalibrierung
    dsp.cpp             Hochpass, Bandpass-Biquads, RMS/Peak, Sinc-Korrektur
    spectrum.cpp        Hann, FFT, Peak-Interpolation, Oktavbänder
    ui_tft.cpp          Screens, Balken, Zahlen
    logger.cpp          CSV auf SD
    config.h            Pins, fs-Default, Filtergrenzen, Kalibrieroffset
  test/
    test_dsp.cpp        Unit-Tests gegen synthetische Sinus-Signale
tools/
  spl_calc.py           Auslegungsrechner
  dsp_model.py          Referenzmodell der Messkette (Phase 1)
  test_dsp_model.py     99 Abnahmetests fuer das Modell
  export_reference.py   erzeugt reference_vectors.h
  verify_log.py         Auswertung der CSV-Logs am PC
```

## Verifikation der DSP-Kette ohne Hardware

Erledigt in Phase 1: das Python-Referenzmodell erreicht im Tonpfad über das
gesamte Band **±0,03 dB und ±0,01 Hz**, Linearität über 110 … 178 dB besser als
0,1 dB. Details in [06-phase1-ergebnisse.md](06-phase1-ergebnisse.md).

`tools/export_reference.py` erzeugt daraus `firmware/test/reference_vectors.h`:
Butterworth-Gütefaktoren, alle Biquad-Koeffizienten für fs = 500 und 622 Hz in
beiden Bändern, Sollfrequenzgang an sechs Stützstellen, einen goldenen Vektor
aus 64 Ein-/Ausgangswerten der Gesamtkette sowie stationäre Sollwerte für
sieben Prüffrequenzen.

`test/test_dsp.cpp` muss diese Werte reproduzieren. Die C++-Portierung ist
damit reine Übersetzungsarbeit mit objektivem Abnahmekriterium, statt einer
zweiten Implementierung, die man wieder neu glauben muss.
