# Abarbeitungsliste

Reihenfolge ist bewusst so gewählt, dass alles, was ohne Hardware prüfbar ist,
vor der ersten Bestellung geklärt wird. Jede Phase hat ein Abnahmekriterium —
erst wenn das erfüllt ist, geht es weiter.

---

## Phase 0 — Entscheidungen und Bestellung  (~1 Tag, 55-95 EUR)

- [ ] 0.1 Sensor festlegen: BMP581 (Standard) oder BMP390 (Fallback). Verfügbarkeit und Preis prüfen.
- [ ] 0.2 Display festlegen: ESP32-2432S028R mit 2,8" TFT (Standard) oder 1,3" OLED (bessere Sonnenlesbarkeit, kleinere Grafik).
- [ ] 0.3 Bestellung auslösen — **Einkaufsliste mit Bezugsquellen: [docs/08-einkaufsliste.md](docs/08-einkaufsliste.md)**. Lieferzeit AliExpress 2-4 Wochen einplanen.
- [ ] 0.4 Zielspezifikation schriftlich fixieren: Messbereich 95-178 dB, Band 10-100 Hz, Anzeige RMS + Peak + f0.

**Abnahme:** Bestellung raus, Spezifikation steht.

---

## Phase 1 — DSP am PC verifizieren  ✅ **abgeschlossen**

- [x] 1.1 `tools/spl_calc.py` durchgerechnet, gegen `docs/01-messprinzip.md` gegengeprüft.
- [x] 1.2 Referenzmodell der kompletten Kette gebaut (`tools/dsp_model.py`): DC-Blocker, umschaltbares Bandfilter, Hann-FFT, Peak-Interpolation, RMS/Peak-Detektoren. Ohne numpy/scipy, sample-by-sample — direkt nach C++ portierbar.
- [x] 1.3 Mit synthetischen Signalen getestet: Einzeltöne 10-100 Hz, Mehrton, Luftdruckdrift, Offsetvariation, float32-Quantisierung, Bandumschaltung.
- [x] 1.4 Geprüft: **Tonpfad ±0,03 dB und ±0,01 Hz** über das ganze Band, Linearität 110-178 dB besser 0,1 dB, Bandfilter schwingt in 0,019 s ein. Alle Vorgaben übertroffen.
- [x] 1.5 `tools/export_reference.py` erzeugt `firmware/test/reference_vectors.h` als Sollwertsatz für die C++-Portierung. **Keine** feste Koeffiziententabelle für die Firmware — siehe Befund 3.

**Abnahme erfüllt:** 99 von 99 Tests bestanden (`python3 tools/test_dsp_model.py`). Ergebnisse und Befunde in [docs/06-phase1-ergebnisse.md](docs/06-phase1-ergebnisse.md).

Drei Befunde, die in die Firmware einfließen:
1. Die FFT gehört **vor** das Bandfilter, sonst erbt die Frequenzanzeige die Filterflanke (3 dB Fehler an den Bandgrenzen). Peak-Suche zusätzlich ein Bin über die Grenzen hinaus.
2. Peak-Hold braucht eine **Einschwingsperre von 150 ms**, sonst landet der Überschwinger des Bandfilters als Spitzenpegel im Display (+4,10 statt +3,01 dB über RMS).
3. Biquad-Koeffizienten **zur Laufzeit** aus der gemessenen Abtastrate berechnen, nicht als Tabelle hinterlegen — der RC-Oszillator des Sensors weicht bis ±5 % ab.

---

## Phase 2 — Sensor-Bringup  (~1 Tag, nach Lieferung)

- [ ] 2.1 ESP32-Board in PlatformIO einrichten (`firmware/platformio.ini` steht), Display-Beispiel (TFT_eSPI) zum Laufen bringen.
- [ ] 2.2 BMP581 per I²C anbinden, Chip-ID lesen, Absolutdruck plausibel (800-1100 hPa)?
- [ ] 2.3 Continuous Mode, OSR 1x, IIR aus, FIFO aktivieren.
- [ ] 2.4 FIFO-Burst-Read alle 40 ms, Overrun-Flag auswerten.
- [ ] 2.5 **fs-Kalibrierung**: über 30 s Samples zählen, reale Abtastrate bestimmen und in NVS speichern. Gerüst in `src/main.cpp`, TODOs markiert. Bei Overrun muss die Zählung neu starten, sonst kommt die Rate zu niedrig heraus.
- [ ] 2.6 Rohdaten über USB-Seriell streamen und am PC mit `tools/verify_log.py` plotten.

**Abnahme:** Lückenloser Datenstrom mit bekannter, stabiler Abtastrate (Schwankung < 0,5 %), keine FIFO-Overruns über 10 Minuten.

---

## Phase 3 — Messkette auf dem ESP32  (~2 Tage)

Der hardwareunabhängige Teil ist **vorgezogen und fertig** — er brauchte kein Board.

- [x] 3.1 Python-Modell nach C++ portiert (`firmware/src/dsp.*`, `spectrum.*`, `meter.*`). Frei von Arduino-Abhängigkeiten, damit derselbe Code im PC-Test läuft. FFT vor dem Bandfilter.
- [x] 3.2 Abnahmetest gegen `reference_vectors.h`: **142 Tests grün**, goldener Vektor auf 0,007 Pa reproduziert. `make -C firmware test`, nur g++ nötig.
- [x] 3.3 ~~ESP-DSP-FFT~~ **nicht nötig.** Eigene Radix-2-FFT mit vorberechneten Drehfaktoren. Bei N=1024 und Hop N/4 läuft sie 2,4-mal je Sekunde; gemessener Durchsatz 0,14 µs pro Sample, das sind unter 1 % Rechenzeit auf dem ESP32. Spart eine Abhängigkeit und macht den Code nativ testbar.
- [x] 3.4 Sinc-Korrektur (pro Bin im Spektralpfad, über f0 im Detektorpfad) und Peak-Interpolation, Peak-Suche ein Bin über die Bandgrenzen hinaus.
- [x] 3.4a Biquad-Auslegung zur Laufzeit aus der gemessenen Abtastrate.
- [x] 3.4b Einschwingsperre (150 ms) für Peak- und Max-Hold nach Start, Reset und Bandwechsel.
- [x] 3.4c **Neuer Befund:** obere Bandgrenze ≤ 0,4 × Abtastrate erzwungen. Genau auf Nyquist entartet der Tiefpass; ein unzulässiges Band wird jetzt abgelehnt statt still kaputt gebaut. Folge: BMP390 mit 200 Hz schafft höchstens 80 Hz obere Grenze.
- [ ] 3.5 Erster Realtest: Sub mit Sinus 30 Hz ansteuern, Pegel schrittweise erhöhen, Linearität prüfen (10 dB mehr Anregung → 10 dB mehr Anzeige). **Braucht Hardware.**
- [ ] 3.6 CPU-Last und Task-Timing auf dem Board bestätigen; Sensor-Task auf Kern 0 mit hoher Priorität, Anzeige auf Kern 1. Gerüst steht in `src/main.cpp`. **Braucht Hardware.**

**Abnahme (PC-Teil) erfüllt:** 142 von 142 Tests bestanden. Begründung der Sprachwahl und Messwerte in [docs/07-warum-cpp.md](docs/07-warum-cpp.md).
**Abnahme (Board-Teil) offen:** Angezeigter Pegel folgt linear der Anregung, f0 stimmt mit dem Generator auf < 0,2 Hz überein.

---

## Phase 4 — Bedienoberfläche  (~2 Tage)

- [ ] 4.1 Screen LIVE: große dB-Zahl, f0, Balkenspektrum 10-100 Hz.
- [ ] 4.2 Screen PEAK/BURP: max. RMS groß, True-Peak als Nebenwert, f0 beim Maximum, Reset per langem Tastendruck.
- [ ] 4.2a Bandumschaltung 10-100 / 5-150 Hz im Menü; Wechsel muss die Haltewerte zurücksetzen.
- [ ] 4.3 Screen RTA: 1/6-Oktav-Balken mit Max-Hold, N=1024.
- [ ] 4.4 Statuszeile: `dB SPL (Z, 10-100 Hz)`, fs, Temperatur, OVER-/Overrun-Warnung.
- [ ] 4.4a Debug-/Kalibrierscreen: zeigt `rawPressurePa` (Absolutdruck vor dem DC-Blocker) und die Sensortemperatur. Ohne den ist der Wassersäulen-Test aus Phase 5.1 nicht durchführbar.
- [ ] 4.5 Modusumschaltung per Taster/Touch, Einstellungen in NVS persistent.
- [ ] 4.6 Ablesbarkeit prüfen: aus 1,5 m Entfernung bei Tageslicht lesbar? Sonst Schriftgröße/Kontrast anpassen.

**Abnahme:** Alle Modi bedienbar, Anzeige auch bei Erschütterung stabil, Refresh ≥ 4 Hz.

---

## Phase 5 — Kalibrierung und Verifikation  (~1 Tag)

- [ ] 5.0 Sofort-Check vor allem anderen: Gerät 10 m höher tragen, Rohdruck muss um ~120 Pa fallen. Deckt grobe Skalierungsfehler in zwei Minuten auf.
- [ ] 5.1 Wassersäulen-Test (`docs/05-kalibrierung.md` A): dichte Sensorkammer bauen, Reihe über 10/20/50 cm, Hauptmesspunkt 50 cm. Abweichung < 1 %.
- [ ] 5.2 Rauschteppich im ruhigen Raum messen und dokumentieren.
- [ ] 5.3 Frequenzgang mit DIY-Pistonphon oder Zweitsensor-Vergleich (10-100 Hz, ±0,5 dB erwartet).
- [ ] 5.4 Clipping-Grenze nachrechnen und Warnschwelle in der Firmware setzen (175 dB).
- [ ] 5.5 `docs/kalibrierprotokoll.md` ausfüllen.

**Abnahme:** Gain auf ±1 % bestätigt, Frequenzgang dokumentiert, Grenzen bekannt.

---

## Phase 6 — Gehäuse  (~2-3 Tage inkl. Druckzeit) — Modell fertig, Rest braucht Hardware

- [x] 6.1 OpenSCAD-Modell erstellt (`case/`): Rückschale mit belüfteter Sensorkammer, Frontblende, Kalibrierkammer, Passungslehre, 1:1-Papiervorlage. Alle STLs als manifold geprüft.
- [ ] 6.1a **Platine vermessen** und die `MESSEN`-Werte in `case/params.scad` eintragen. Die Startwerte sind geraten — die Netzangaben zum Board widersprechen sich.
- [ ] 6.2 Papiervorlage bei 100 % ausdrucken, Platine drauflegen. Dann Passungslehre drucken (`make -C case fit`, ~15 min), erst danach das Gehäuse (~6 h).
- [ ] 6.3 Vollständiger Druck in ASA/PETG, Gewindeeinsätze einschmelzen.
- [ ] 6.4 Montage, Sensor auf Moosgummi entkoppeln, Schaumstoff über den Port.
- [ ] 6.5 **Frequenzgang nach dem Einbau erneut messen** (Phase 5.3 wiederholen) — das Gehäuse ist der wahrscheinlichste Fehlerort.
- [ ] 6.6 STL-Export nach `case/stl/`.

**Abnahme:** Frequenzgang im Gehäuse weicht < 1 dB vom Freiluftwert ab. Sonst: Port vergrößern, Kammervolumen verkleinern.

---

## Phase 7 — Fahrzeugeinbau und Feldtest  (~1 Tag)

- [ ] 7.1 Spannungsversorgung aufbauen: Sicherung, Verpolschutz, TVS, DC/DC. Ohne angeschlossenes Board erst die 5 V messen.
- [ ] 7.2 Sternmasse anlegen, nicht an der Endstufenmasse.
- [ ] 7.3 Einbau, Messposition festlegen und dokumentieren.
- [ ] 7.4 Störtest: Anlage auf Vollgas, dabei prüfen, ob Messwerte plausibel bleiben und das Board nicht resettet.
- [ ] 7.5 Sweep 10-100 Hz fahren, Kurve loggen, mit der bekannten Abstimmung des Gehäuses vergleichen (Impedanzminimum/Tuningfrequenz sollte sich als Pegelmaximum zeigen).
- [ ] 7.6 Wenn möglich: Gegenmessung mit einem kommerziellen Gerät.

**Abnahme:** Reproduzierbare Messwerte (Wiederholung derselben Messung < 1 dB Streuung), keine elektrischen Störungen.

---

## Phase 8 — Optional / Ausbau

- [ ] 8.1 CSV-Logging auf microSD, Auswertung mit `tools/verify_log.py`.
- [ ] 8.2 WLAN-AP mit Web-Oberfläche: Live-Spektrum und Download der Logs am Handy.
- [ ] 8.3 OTA-Update.
- [ ] 8.4 Zweiter Sensor (BMP390) als Dauer-Plausibilitätsprüfung.
- [ ] 8.5 Auto-Dimming des Displays über den onboard LDR.
- [ ] 8.6 Gesamtdoku und Bauanleitung als Release veröffentlichen.

---

## Zeit- und Kostenrahmen

| | |
|---|---|
| Aktive Arbeitszeit | ca. 10-13 Tage, gut in Abendetappen machbar (Phase 1 erledigt) |
| Wartezeit Lieferung | 2-4 Wochen (parallel zu Phase 1) |
| Materialkosten | 55 EUR (AliExpress) bis 95 EUR (deutsche Shops) |

## Größte Risiken

| Risiko | Gegenmaßnahme |
|---|---|
| Gehäuse dämpft tiefe Frequenzen | Offener Port, kleine Kammer, Messung in Phase 6.5 |
| Falsche Abtastrate → falsche Frequenzanzeige | fs-Kalibrierung in Phase 2.5, Pflichtschritt |
| Bordnetzstörungen / Resets | Schutzbeschaltung Phase 7.1, Sternmasse |
| Sensor am Anschlag bei Extrempegeln | Clipping-Erkennung Phase 5.4, Grenze bei 178 dB |
| Vibration verfälscht Messung | Sensorentkopplung Phase 6.4 |
