# Abarbeitungsliste

Reihenfolge ist bewusst so gewählt, dass alles, was ohne Hardware prüfbar ist,
vor der ersten Bestellung geklärt wird. Jede Phase hat ein Abnahmekriterium —
erst wenn das erfüllt ist, geht es weiter.

---

## Phase 0 — Entscheidungen und Bestellung  (~1 Tag, 55-85 EUR)

- [ ] 0.1 Sensor festlegen: BMP581 (Standard) oder BMP390 (Fallback). Verfügbarkeit und Preis prüfen.
- [ ] 0.2 Display festlegen: ESP32-2432S028R mit 2,8" TFT (Standard) oder 1,3" OLED (bessere Sonnenlesbarkeit, kleinere Grafik).
- [ ] 0.3 Bestellung auslösen (Stückliste in `docs/02-hardware.md`). Lieferzeit AliExpress 2-4 Wochen einplanen — deshalb zuerst bestellen, dann weiterarbeiten.
- [ ] 0.4 Zielspezifikation schriftlich fixieren: Messbereich 95-178 dB, Band 10-100 Hz, Anzeige RMS + Peak + f0.

**Abnahme:** Bestellung raus, Spezifikation steht.

---

## Phase 1 — DSP am PC verifizieren  (~1-2 Tage, 0 EUR, keine Hardware nötig)

- [ ] 1.1 `tools/spl_calc.py` durchrechnen, Ergebnisse gegen `docs/01-messprinzip.md` gegenprüfen.
- [ ] 1.2 Python-Referenzmodell der kompletten Kette schreiben (`tools/dsp_model.py`): Hochpass, Bandpass 5-150 Hz, Hann-FFT, Peak-Interpolation, RMS/Peak-Detektor.
- [ ] 1.3 Mit synthetischen Signalen testen: Sinus 10/20/40/100 Hz bei bekannter Amplitude in Pa, plus Rauschen, plus Luftdruckdrift.
- [ ] 1.4 Prüfen: SPL-Fehler < 0,1 dB, f0-Fehler < 0,1 Hz, Einschwingzeit des Bandpasses akzeptabel (< 0,3 s).
- [ ] 1.5 Biquad-Koeffizienten für fs = 500 Hz und 622 Hz erzeugen und als `config.h`-Tabelle exportieren.

**Abnahme:** Modell liefert für alle Testsignale Werte innerhalb der Toleranz. Damit ist die Mathematik gesichert, bevor Hardware da ist.

---

## Phase 2 — Sensor-Bringup  (~1 Tag, nach Lieferung)

- [ ] 2.1 ESP32-Board in PlatformIO einrichten, Display-Beispiel (TFT_eSPI) zum Laufen bringen.
- [ ] 2.2 BMP581 per I²C anbinden, Chip-ID lesen, Absolutdruck plausibel (800-1100 hPa)?
- [ ] 2.3 Continuous Mode, OSR 1x, IIR aus, FIFO aktivieren.
- [ ] 2.4 FIFO-Burst-Read alle 40 ms, Overrun-Flag auswerten.
- [ ] 2.5 **fs-Kalibrierung**: über 30 s Samples zählen, reale Abtastrate bestimmen und in NVS speichern.
- [ ] 2.6 Rohdaten über USB-Seriell streamen und am PC mit `tools/verify_log.py` plotten.

**Abnahme:** Lückenloser Datenstrom mit bekannter, stabiler Abtastrate (Schwankung < 0,5 %), keine FIFO-Overruns über 10 Minuten.

---

## Phase 3 — Messkette auf dem ESP32  (~2 Tage)

- [ ] 3.1 Python-Modell aus Phase 1 nach C++ portieren (`dsp.cpp`, `spectrum.cpp`).
- [ ] 3.2 PlatformIO-Native-Tests (`test/test_dsp.cpp`) mit denselben Testsignalen wie Phase 1 — C++ muss dieselben Werte liefern wie Python.
- [ ] 3.3 ESP-DSP-FFT einbinden, N=512, Hann, Hop N/4.
- [ ] 3.4 Sinc-Korrektur und Peak-Interpolation ergänzen.
- [ ] 3.5 Erster Realtest: Sub mit Sinus 30 Hz ansteuern, Pegel schrittweise erhöhen, Linearität prüfen (10 dB mehr Anregung → 10 dB mehr Anzeige).
- [ ] 3.6 CPU-Last und Task-Timing messen, Sensor-Task auf eigenen Kern mit hoher Priorität.

**Abnahme:** Angezeigter Pegel folgt linear der Anregung, f0 stimmt mit dem Generator auf < 0,2 Hz überein.

---

## Phase 4 — Bedienoberfläche  (~2 Tage)

- [ ] 4.1 Screen LIVE: große dB-Zahl, f0, Balkenspektrum 10-100 Hz.
- [ ] 4.2 Screen PEAK/BURP: Max-Hold groß, f0 beim Maximum, Reset per langem Tastendruck.
- [ ] 4.3 Screen RTA: 1/6-Oktav-Balken mit Max-Hold, N=1024.
- [ ] 4.4 Statuszeile: `dB SPL (Z, 10-100 Hz)`, fs, Temperatur, OVER-/Overrun-Warnung.
- [ ] 4.5 Modusumschaltung per Taster/Touch, Einstellungen in NVS persistent.
- [ ] 4.6 Ablesbarkeit prüfen: aus 1,5 m Entfernung bei Tageslicht lesbar? Sonst Schriftgröße/Kontrast anpassen.

**Abnahme:** Alle Modi bedienbar, Anzeige auch bei Erschütterung stabil, Refresh ≥ 4 Hz.

---

## Phase 5 — Kalibrierung und Verifikation  (~1 Tag)

- [ ] 5.1 Wassersäulen-Test (`docs/05-kalibrierung.md` A): 5/10/20 cm, Abweichung < 1 %.
- [ ] 5.2 Rauschteppich im ruhigen Raum messen und dokumentieren.
- [ ] 5.3 Frequenzgang mit DIY-Pistonphon oder Zweitsensor-Vergleich (10-100 Hz, ±0,5 dB erwartet).
- [ ] 5.4 Clipping-Grenze nachrechnen und Warnschwelle in der Firmware setzen (175 dB).
- [ ] 5.5 `docs/kalibrierprotokoll.md` ausfüllen.

**Abnahme:** Gain auf ±1 % bestätigt, Frequenzgang dokumentiert, Grenzen bekannt.

---

## Phase 6 — Gehäuse  (~2-3 Tage inkl. Druckzeit)

- [ ] 6.1 OpenSCAD-Modell nach `docs/04-gehaeuse.md` erstellen (`case/`).
- [ ] 6.2 Testdruck nur der Sensorkammer, Passung des Sensorports prüfen.
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
| Aktive Arbeitszeit | ca. 10-13 Tage, gut in Abendetappen machbar |
| Wartezeit Lieferung | 2-4 Wochen (parallel zu Phase 1) |
| Materialkosten | 55-85 EUR |

## Größte Risiken

| Risiko | Gegenmaßnahme |
|---|---|
| Gehäuse dämpft tiefe Frequenzen | Offener Port, kleine Kammer, Messung in Phase 6.5 |
| Falsche Abtastrate → falsche Frequenzanzeige | fs-Kalibrierung in Phase 2.5, Pflichtschritt |
| Bordnetzstörungen / Resets | Schutzbeschaltung Phase 7.1, Sternmasse |
| Sensor am Anschlag bei Extrempegeln | Clipping-Erkennung Phase 5.4, Grenze bei 178 dB |
| Vibration verfälscht Messung | Sensorentkopplung Phase 6.4 |
