# 6. Ergebnisse Phase 1 — DSP-Modell am PC

Status: **abgeschlossen**. 99 von 99 Abnahmetests bestanden.

```bash
python3 tools/dsp_model.py         # Kennwerttabellen und Demo
python3 tools/test_dsp_model.py    # Abnahmetests, Exitcode 0 = bestanden
python3 tools/export_reference.py  # Referenzwerte fuer die C++-Portierung
```

Das Modell ist bewusst ohne numpy/scipy und strikt sample-by-sample
geschrieben. Es ist damit direkt die Vorlage für die C++-Portierung; jede
Funktion hat ein Gegenstück in `firmware/src/`.

## Festgelegte Parameter

| Parameter | Wert | Begründung |
|---|---|---|
| DC-Blocker | Hochpass 1. Ordnung, 0,5 Hz | entfernt 101 kPa Luftdruck und Wetterdrift |
| Messband A | 10–100 Hz | vergleichbar mit kommerziellen Bass-Metern |
| Messband B | 5–150 Hz | erfasst Infraschall und unteren Kickbass |
| Bandfilter | Butterworth 4. Ordnung HP + LP | −3 dB an den Bandgrenzen, 24 dB/Oktave |
| Detektor Fast | τ = 125 ms | wie bei Schallpegelmessern |
| Detektor Slow | τ = 1 s | stabiler Ablesewert |
| Einschwingsperre | 150 ms | siehe Befund 2 |
| FFT | Hann, N = 512 (LIVE) / 1024 (RTA), Hop N/4 | 1,21 / 0,61 Hz Bins bei fs = 622 Hz |

## Genauigkeit der Gesamtkette

Sinus 150,0 dB, fs = 622 Hz, Band 10–100, mit nachgebildeter Sensormittelung:

| f [Hz] | Detektorpfad | Fehler | Tonpfad (FFT) | Fehler | f0 [Hz] | Fehler |
|---|---|---|---|---|---|---|
| 10,0 | 146,97 | −3,03 | 149,97 | −0,03 | 10,003 | +0,003 |
| 20,0 | 149,94 | −0,06 | 149,98 | −0,02 | 19,996 | −0,004 |
| 31,5 | 149,97 | −0,03 | 149,98 | −0,02 | 31,493 | −0,007 |
| 50,0 | 149,97 | −0,03 | 149,98 | −0,02 | 50,010 | +0,010 |
| 63,0 | 149,91 | −0,09 | 149,98 | −0,02 | 62,990 | −0,010 |
| 80,0 | 149,45 | −0,55 | 149,98 | −0,02 | 79,990 | −0,010 |
| 100,0 | 146,97 | −3,03 | 149,98 | −0,02 | 99,991 | −0,009 |

**Zielvorgabe Phase 1 (< 0,1 dB, < 0,1 Hz) im Tonpfad über das gesamte Band
erreicht.** Der Detektorpfad liegt an den Bandgrenzen definitionsgemäß 3 dB
tiefer — das ist die Bandgrenze selbst, kein Fehler.

Linearität über 110 … 178 dB: Fehler < 0,1 dB an jedem Prüfpunkt.

## Einschwingzeiten

| | Band 10–100 | Band 5–150 |
|---|---|---|
| Bandfilter allein (99 % Amplitude) | 0,019 s | 0,018 s |
| Detektor Fast (±1 dB) | 0,198 s | 0,196 s |
| Detektor Slow (±1 dB) | 1,608 s | 1,596 s |

Das Filter selbst schwingt in 19 ms ein, weit unter der Vorgabe von 0,3 s. Die
sichtbare Trägheit kommt allein von den Detektorzeitkonstanten und ist gewollt.

## Drei Befunde, die die Firmware betreffen

### Befund 1 — FFT gehört **vor** das Bandfilter

Ursprünglich lief die FFT hinter dem Bandfilter. Ein Ton genau auf einer
Bandgrenze wurde dadurch 3 dB zu niedrig angezeigt, weil die Frequenzanzeige
die Filterflanke erbte.

Jetzt bekommt die FFT das nur DC-befreite Signal. Die Bandgrenzen wirken im
Spektralpfad rechteckig über die Bin-Auswahl. Ergebnis: Tonpfad über das ganze
Band auf ±0,03 dB genau, siehe Tabelle oben.

Zusätzlich muss die Peak-Suche **ein Bin über die Bandgrenzen hinaus** laufen.
Sonst liegt bei einem Ton exakt auf der Grenze das wahre Maximum im ersten Bin
außerhalb des Suchbereichs und die Interpolation läuft in ihre Begrenzung — im
Test ein Fehler von 0,079 Hz bei 100 Hz, jetzt 0,009 Hz.

### Befund 2 — Peak-Hold braucht eine Einschwingsperre

Ein Butterworth-Bandfilter überschwingt beim Einschwingen um gut 1 dB. Ohne
Sperre landet dieser Überschwinger als vermeintlicher Spitzenpegel im
Peak-Hold: der Test las **+4,10 dB statt +3,01 dB** über RMS, und das Gerät
hätte dauerhaft einen zu hohen Wert angezeigt.

Abhilfe: nach Start, Peak-Reset und Bandwechsel werden die Haltewerte 150 ms
lang eingefroren (`HOLD_GUARD`). Danach liest der Test 153,03 dB gegen 153,01
dB Sollspitze.

**Für die Firmware heißt das:** der Bandwechsel im Menü muss `reset_hold()`
auslösen, sonst schleppt der Peak-Wert einen Filterartefakt mit.

### Befund 3 — feste Koeffiziententabellen wären falsch

Der Sensor taktet mit einem internen RC-Oszillator, die reale Abtastrate weicht
um bis zu ±5 % vom Nennwert ab. Eine im Code fest hinterlegte
Koeffiziententabelle für „622 Hz" wäre um genau diesen Fehler daneben —
sowohl in den Filtergrenzen als auch in der Frequenzachse der FFT.

Deshalb: **die Firmware berechnet die Biquad-Koeffizienten beim Start aus der
gemessenen Abtastrate.** Das sind zwei `sin`/`cos`-Aufrufe pro Sektion, also
acht insgesamt — vernachlässigbar. `tools/export_reference.py` erzeugt keine
Firmware-Tabelle, sondern die Sollwerte für den C++-Unittest.

## Weitere abgesicherte Eigenschaften

| Prüfung | Ergebnis |
|---|---|
| Luftdruckdrift 200 Pa/s | Pegeländerung < 0,0001 dB |
| Absolutdruck 850 … 1080 hPa | Pegeländerung < 0,0001 dB |
| float32 mit vollem Luftdruckoffset | Abweichung < 0,0001 dB bei 120 dB |
| float32-Quantisierung bei 101325 Pa | 0,0078 Pa Schrittweite = 41 dB SPL, liegt 55 dB unter dem Sensorrauschen |
| Interpolationsbias | max. 0,0155 Bin = 0,0094 Hz |
| Zwei gleich starke Töne | Bandpegel +3,01 dB, wie erwartet |
| Mehrton 35 Hz / 70 Hz (−10 dB) | f0 = 34,992 Hz, Grundton 150,02 dB, Bandpegel 150,45 dB (Soll 150,41) |
| Band 10–100 gegen 130 Hz | −12,5 dB |
| BMP390 (200 Hz) mit Band 5–150 | wird mit Fehler abgelehnt — obere Grenze über Nyquist |

Die float32-Prüfung entschärft eine Sorge aus der Planung: der volle
Luftdruckoffset in float32 ist unkritisch, eine Vorab-Subtraktion im
Integerbereich ist nicht zwingend nötig. Sie bleibt trotzdem sinnvoll, weil sie
nichts kostet.

## Übergabe an Phase 3

`firmware/test/reference_vectors.h` enthält:

- Butterworth-Gütefaktoren
- alle Biquad-Koeffizienten für fs = 500 und 622 Hz, beide Bänder
- Sollfrequenzgang an sechs Stützstellen je Band
- goldener Vektor: 64 Ein-/Ausgangswerte der Gesamtkette
- stationäre Sollwerte für sieben Prüffrequenzen
- Zeitkonstanten und Sperrzeiten

Der C++-Test in Phase 3 muss diese Werte reproduzieren. Damit ist die
Portierung eine reine Übersetzungsarbeit mit objektivem Abnahmekriterium.
