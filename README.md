# Bass-SPL-Meter (10-100 Hz)

Selbstbau-Messgerät für Schalldruckpegel im Subbassbereich — für Showcars und
Car-Audio-Anlagen. Zeigt kalibrierten Pegel in dB SPL und die dominante
Frequenz an, vergleichbar mit kommerziellen Geräten wie dem Basshead Garage
BHG-DB.

## Eckdaten

| | |
|---|---|
| Messbereich | ca. 95 … 178 dB SPL |
| Frequenzbereich | 10 … 100 Hz (bis 150 Hz nutzbar) |
| Frequenzauflösung | 0,98 Hz (FFT), f0 auf < 0,1 Hz interpoliert |
| Anzeige | 2,8" TFT: Pegel, Peak-Hold, dominante Frequenz, Spektrum |
| Messwandler | Absolut-Drucksensor BMP581 (kein Mikrofon) |
| Rechner | ESP32 |
| Versorgung | 9-18 V Bordnetz |
| Materialkosten | 55-85 EUR |

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
| **[TASKS.md](TASKS.md)** | **Abarbeitungsliste in 9 Phasen mit Abnahmekriterien** |

## Werkzeuge

```bash
python3 tools/spl_calc.py      # Auslegungsrechner: Pegel/Druck, Sensorreserve, FFT-Parameter
```

## Status

Planungsphase. Als Nächstes: Phase 0 (Bestellung) und Phase 1 (DSP-Modell am
PC) aus [TASKS.md](TASKS.md).

## Hinweis

Kein geeichtes Messgerät. Keine Zertifizierung nach IEC 61672, nicht für
offizielle Wettbewerbsergebnisse geeignet. Details in
[docs/05-kalibrierung.md](docs/05-kalibrierung.md), Abschnitt E.
