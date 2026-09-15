# Bass-SPL-Meter (10-100 Hz)

Selbstbau-Messgerät für Schalldruckpegel im Subbassbereich — für Showcars und
Car-Audio-Anlagen. Zeigt kalibrierten Pegel in dB SPL und die dominante
Frequenz an, vergleichbar mit kommerziellen Geräten wie dem Basshead Garage
BHG-DB.

## Eckdaten

| | |
|---|---|
| Messbereich | ca. 95 … 178 dB SPL |
| Frequenzbereich | 10 … 100 Hz, im Menü auf 5 … 150 Hz umschaltbar |
| Frequenzauflösung | 0,6-1,2 Hz (FFT), f0 auf < 0,01 Hz interpoliert |
| Anzeige | 2,8" TFT: Pegel, Peak-Hold, dominante Frequenz, Spektrum |
| Messwandler | Absolut-Drucksensor BMP581 (kein Mikrofon) |
| Rechner | ESP32 |
| Versorgung | 9-18 V Bordnetz |
| Materialkosten | 55 EUR (AliExpress) bis 95 EUR (deutsche Shops) |

## Grundidee

Ein Mikrofon scheidet aus: bezahlbare Kapseln clippen ab ~130 dB und sind unter
20 Hz taub. Ein barometrischer Absolut-Drucksensor misst dagegen bis ~178 dB,
reicht bis 0 Hz hinunter und liefert den Messwert **direkt in Pascal** — die
Kalibrierung ist damit ab Werk gegeben, statt von einer unbekannten
Mikrofonempfindlichkeit abzuhängen.

## Dokumentation

| Datei | Inhalt |
|---|---|
| [docs/01-messprinzip.md](docs/01-messprinzip.md) | Warum Drucksensor statt Mikrofon, Physik, Sensorauswahl, Aliasing |
| [docs/02-hardware.md](docs/02-hardware.md) | Stückliste mit Preisen, Blockschaltbild, Pinbelegung, Bordnetzschutz |
| [docs/03-firmware.md](docs/03-firmware.md) | Signalkette, FFT-Parameter, Betriebsmodi, Codestruktur |
| [docs/04-gehaeuse.md](docs/04-gehaeuse.md) | 3D-Druck, Material, Druckport, Entkopplung, Montage |
| [docs/05-kalibrierung.md](docs/05-kalibrierung.md) | Gain-Prüfung per Wassersäule, Frequenzgang, Grenzen des Geräts |
| [docs/06-phase1-ergebnisse.md](docs/06-phase1-ergebnisse.md) | Messergebnisse des DSP-Modells und die Befunde daraus |
| [docs/07-warum-cpp.md](docs/07-warum-cpp.md) | Warum die Gerätesoftware C++ ist und Python die Referenz bleibt |
| [docs/08-einkaufsliste.md](docs/08-einkaufsliste.md) | Bezugsquellen, Artikelnummern, zwei Bestellwege |
| [firmware/README.md](firmware/README.md) | Aufbau der Firmware, Testen ohne Hardware |
| [case/README.md](case/README.md) | Gehäusemodell, Messliste, Druckeinstellungen |
| **[TASKS.md](TASKS.md)** | **Abarbeitungsliste in 9 Phasen mit Abnahmekriterien** |

## Werkzeuge

```bash
python3 tools/spl_calc.py          # Auslegung: Pegel/Druck, Sensorreserve, FFT-Parameter
python3 tools/dsp_model.py         # Referenzmodell der Messkette, Kennwerttabellen
python3 tools/test_dsp_model.py    # 99 Abnahmetests, Exitcode 0 = bestanden
python3 tools/export_reference.py  # Sollwerte fuer die C++-Portierung erzeugen
```

Alles reine Standardbibliothek — kein numpy, kein scipy, keine Installation.

```bash
make -C firmware test   # 142 Abnahmetests der C++-Portierung, nur g++ noetig
make -C firmware bench  # Durchsatzmessung
```

## Status

**Phase 1 abgeschlossen, Phase 3 vorgezogen, soweit ohne Hardware möglich.**

- DSP-Referenzmodell in Python: Pegelfehler ±0,03 dB, Frequenzfehler ±0,01 Hz
  über das gesamte Messband, 99 Tests → [docs/06-phase1-ergebnisse.md](docs/06-phase1-ergebnisse.md)
- C++-Portierung für den ESP32: 151 Tests gegen die Referenzwerte des Modells,
  läuft ohne Board → [firmware/README.md](firmware/README.md)
- Parametrisches Gehäusemodell mit Passungslehre und 1:1-Papiervorlage
  → [case/README.md](case/README.md)

Als Nächstes: **Phase 0 (Bestellung)** — sie blockiert alles Weitere. Danach
Phase 2 (Sensor-Bringup) aus [TASKS.md](TASKS.md).

## Hinweis

Kein geeichtes Messgerät. Keine Zertifizierung nach IEC 61672, nicht für
offizielle Wettbewerbsergebnisse geeignet. Details in
[docs/05-kalibrierung.md](docs/05-kalibrierung.md), Abschnitt E.
