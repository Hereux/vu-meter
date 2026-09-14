# 3. Firmware

Toolchain: **PlatformIO + Arduino-Framework für ESP32**. Bibliotheken:
`SparkFun_BMP581_Arduino_Library`, `TFT_eSPI`, `ESP-DSP` (FFT), `SD`.

## Signalkette

```
BMP581 FIFO (fs ~500-622 Hz, OSR 1x, IIR aus)
   |  Burst-Read 32 Werte alle ~50 ms
   v
Ringpuffer  [float Pa]
   |
   +-- DC-/Drift-Entfernung: Hochpass 1. Ordnung @ 2 Hz  (entfernt Luftdruck, Türschlagen, Fahrtwind)
   |
   +-- Bandpass Butterworth 4. Ordnung, 5-150 Hz (2 Biquads)
   |
   +-- Sinc-Korrektur (Tabelle aus tools/spl_calc.py)
   |
   +---> Zeitbereich: RMS-Detektor (Fast 125 ms / Slow 1 s) + True-Peak
   |         -> SPL_rms, SPL_peak, Max-Hold
   |
   +---> Frequenzbereich: Hann-Fenster, N=512, Hop N/4
             -> Spektrum 5-150 Hz
             -> Peak-Bin + parabolische Interpolation -> f0 auf ~0,05 Hz genau
```

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
| **PEAK / BURP** | Max-Hold-Wert groß, dazu f0 bei Maximum | True-Peak, Reset per Taster |
| **RTA** | Spektrum 10-100 Hz, 1/6-Oktav-Balken, Max-Hold | N=1024 |
| **LOG** | läuft im Hintergrund | CSV auf microSD: `t,SPL_rms,SPL_peak,f0` @ 4 Hz |

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
  verify_log.py         Auswertung der CSV-Logs am PC
```

## Verifikation der DSP-Kette ohne Hardware

`test/test_dsp.cpp` speist synthetische Signale ein (bekannte Amplitude in Pa,
10/20/50/100 Hz) und prüft, dass der berechnete SPL-Wert auf ±0,1 dB und f0 auf
±0,1 Hz stimmt. Das läuft als PlatformIO-Native-Test auf dem PC — damit ist die
Mathematik schon vor dem ersten Lötpunkt abgesichert.
