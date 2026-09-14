# 8. Einkaufsliste

> **Zur Verlässlichkeit der Links:** Die Shop-Seiten selbst waren aus der
> Arbeitsumgebung nicht abrufbar. Die Links stammen aus Suchergebnissen und
> sind **nicht geöffnet worden** — Preise, Lagerbestand und
> Artikelverfügbarkeit also bitte beim Bestellen selbst prüfen. Bei Reichelt
> ist die **Artikelnummer** das Zuverlässige: die in die Suchmaske eintippen,
> falls ein Link ins Leere läuft.
>
> Preisstand: Schätzung auf Basis der Suchergebnisse, Stand September 2026.

## Korrektur zur bisherigen Kalkulation

Der BMP581 ist in Deutschland teurer als in `02-hardware.md` angesetzt:
**27–29 €** statt der geschätzten 8–19 €. Über AliExpress (DFRobot Fermion)
bleibt es bei etwa 12–15 €, dafür mit 2–4 Wochen Lieferzeit. Das verschiebt die
Gesamtsumme auf **55 € (AliExpress-Weg)** bis **95 € (deutscher Weg)** — beides
noch im Budgetrahmen.

---

## A — Kernteile

Diese beiden bestimmen das Gerät. Alles andere ist austauschbar.

### 1. Rechner mit Display: ESP32-2432S028R ("Cheap Yellow Display")

2,8" TFT 320×240 (ILI9341), Touch, microSD, USB-UART — alles auf einem Board.

| Bezug | Preis | Hinweis |
|---|---|---|
| [AliExpress-Suche](https://de.aliexpress.com/w/wholesale-ESP32-2432S028R.html) | 12–16 € | günstigster Weg, 2–4 Wochen |
| [Amazon-Suche](https://www.amazon.de/s?k=ESP32-2432S028R) | 18–25 € | schnell, oft im Doppelpack |
| [eBay-Listing](https://www.ebay.com/itm/186541372755) | ~14 $ | aus dem Suchergebnis, ungeprüft |

**Auf die genaue Typbezeichnung achten:** `ESP32-2432S028R`. Die Varianten
`S032`, `S035` oder `2432S028` ohne `R` haben andere Displays, andere Pins oder
kapazitiven statt resistiven Touch. Die Pinbelegung in
`firmware/include/config.h` gilt für die `R`-Variante — und auch da gibt es
Revisionen, also vor dem Löten nachmessen.

Referenz-Pinouts zum Gegenprüfen:
[Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r/) ·
[Mischianti](https://mischianti.org/esp32-2432s028-cheap-yellow-display-high-resolution-pinout-datasheet-schema-and-specs/)

### 2. Messwandler: BMP581-Breakout

**Das ist das Herzstück.** Kein Ersatz durch BMP280, BME280 oder BMP180 — die
schaffen die nötige Abtastrate nicht.

| Bezug | Preis | Hinweis |
|---|---|---|
| [Eckstein: SparkFun Pressure Sensor BMP581 (Qwiic)](https://eckstein-shop.de/SparkFun-Pressure-Sensor-BMP581-Qwiic-Compatible-with-Arduino-EN) | 27,31 € | DE-Versand, Lieferzeit dort mit 2–3 Wochen angegeben |
| [Eckstein: SparkFun Micro BMP581 (Qwiic Micro)](https://eckstein-shop.de/SparkFun-Micro-Pressure-Sensor-BMP581-Qwiic-Compatible-with-Arduino-EN) | 28,50 € | kleinere Bauform, passt besser in die Sensorkammer |
| [BerryBase: SparkFun Qwiic BMP581](https://www.berrybase.at/sparkfun-qwiic-bmp581) | ~28 € | Alternative |
| [AliExpress: DFRobot Fermion BMP581](https://www.aliexpress.us/item/3256811635538614.html) | ~12–15 € | günstig, I2C/SPI/I3C, 3,3 V |
| [Adafruit BMP581 STEMMA QT](https://www.adafruit.com/product/6407) | ~13 $ | über Distributor in DE |
| [AliExpress-Suche BMP581](https://de.aliexpress.com/w/wholesale-BMP581.html) | ab ~10 € | Qualität schwankt, Chip-ID nach Erhalt prüfen |

**Empfehlung:** Eins über AliExpress **und** eins aus DE bestellen. 40 € für
zwei Sensoren sind gut angelegt — der Zweitsensor ist die Plausibilitätsprüfung
aus Phase 5.3, und wenn einer defekt ankommt, steht das Projekt nicht 4 Wochen.

Beim Anschließen: **3,3 V, niemals 5 V.** Maximal 3,6 V laut Datenblatt.

---

## B — Stromversorgung und Bordnetzschutz (Reichelt)

Ein Showcar-Bordnetz ist rau. Diese Teile sind billig und verhindern, dass
20 € Elektronik an einer Spannungsspitze sterben.

| Teil | Artikelnr. | ca. | Zweck |
|---|---|---|---|
| [RECOM R-78B50-10](https://www.reichelt.de/de/de/shop/suche/R-78B50-10) — DC/DC 6,5–32 V → 5 V, 1 A, SIP-3 | **159150** | 7 € | Bordnetz auf 5 V. Wide-Input deckt Load-Dump-Spitzen ab |
| [TVS SMCJ24A](https://www.reichelt.de/de/de/shop/suche/SMCJ24A) — unidirektional, 24 V, 1500 W | **290911** | 0,50 € | SMD. Wer lieber bedrahtet lötet: nach **P6KE24A** suchen |
| [Schottky SS34](https://www.reichelt.de/de/de/shop/suche/SS34) o. 1N5822 | — | 0,30 € | Verpolschutz in Reihe, 0,4 V Verlust |
| [KFZ-Flachsicherungshalter](https://www.reichelt.de/de/de/shop/suche/Sicherungshalter%20Flachsicherung) + 1-A-Sicherung | — | 3 € | direkt an der Einspeisung |
| [Elko 470 µF / 35 V](https://www.reichelt.de/de/de/shop/suche/470u%2035V%20radial) | — | 0,40 € | Siebung vor dem Wandler |
| [Elko 100 µF / 16 V](https://www.reichelt.de/de/de/shop/suche/100u%2016V%20radial) | — | 0,15 € | hinter dem Wandler |
| [Ferritperle](https://www.reichelt.de/de/de/shop/suche/Ferritkern%20Drossel) | — | 0,30 € | gegen Endstufen-Ripple |
| [Litze 2×0,75 mm²](https://www.reichelt.de/de/de/shop/suche/Fahrzeugleitung%200%2C75) | — | 3 € | Zuleitung |
| [JST-XH-Set](https://www.reichelt.de/de/de/shop/suche/JST%20XH%20Steckverbinder) | — | 4 € | Steckverbindungen |

**Summe B: ca. 19 €.** Bei Reichelt lohnt eine Sammelbestellung — Versand ~6 €.

Alternativ bei **Pollin** ([Suche DC/DC-Wandler](https://www.pollin.de/search?query=DC%2FDC-Wandler%205V)):
fertige 12→5-V-Module ab ~4 €. Billiger, aber ohne den weiten Eingangsbereich
des Recom-Moduls — für ein Fahrzeug ist der R-78B50 die sicherere Wahl.

---

## C — Mechanik und Gehäuse

| Teil | Bezug | ca. |
|---|---|---|
| **ASA- oder PETG-Filament**, ~100 g | [Amazon ASA](https://www.amazon.de/s?k=ASA+Filament+1.75mm) · [Amazon PETG](https://www.amazon.de/s?k=PETG+Filament+1.75mm) | 3 € anteilig |
| Gewindeeinsätze M3 zum Einschmelzen (Set) | [Amazon](https://www.amazon.de/s?k=Gewindeeinsatz+M3+Einschmelzen+Messing) | 9 € Set |
| Schrauben M3×8 Zylinderkopf | [Reichelt](https://www.reichelt.de/de/de/shop/suche/Zylinderschraube%20M3x8) | 3 € |
| Saugnapfhalterung 30 mm, 2 Stück | [Amazon](https://www.amazon.de/s?k=Saugnapf+Halterung+30mm+Scheibe) | 6 € |
| Mikrofon-Windschutz Schaumstoff (für den Druckport) | [Amazon](https://www.amazon.de/s?k=Mikrofon+Windschutz+Schaumstoff) | 5 € |
| Moosgummi 2 mm, selbstklebend (Sensorentkopplung) | [Amazon](https://www.amazon.de/s?k=Moosgummi+2mm+selbstklebend) | 6 € |

**Summe C: ca. 30 €**, davon vieles Restbestand für spätere Projekte.

---

## D — Kalibrierhilfsmittel (Phase 5)

Für 5 € wird die Gain-Prüfung bis 168 dB möglich — ohne Referenzgerät.

| Teil | Bezug | ca. |
|---|---|---|
| Klarer Silikonschlauch 6 mm, 1 m | [Amazon](https://www.amazon.de/s?k=Silikonschlauch+6mm+transparent) | 5 € |
| Einwegspritze 60 ml (für das Pistonphon) | Apotheke / [Amazon](https://www.amazon.de/s?k=Einwegspritze+60ml) | 3 € |
| Stahllineal 300 mm | vorhanden oder 5 € | — |

---

## Zwei Bestellwege

### Schnell (5–7 Tage, ~95 €)

| | |
|---|---|
| Amazon | CYD-Board, Filament, Kleinteile, Mechanik |
| Eckstein oder BerryBase | BMP581 |
| Reichelt | Bordnetzteile in einer Sammelbestellung |

### Günstig (3–5 Wochen, ~55 €)

| | |
|---|---|
| AliExpress | CYD-Board, BMP581 (Fermion), DC/DC-Modul, Kleinteile |
| Reichelt | nur Sicherungshalter, TVS und Litze |

**Praktischer Vorschlag:** AliExpress für CYD und einen BMP581 sofort
losschicken, parallel die Reichelt-Bestellung aufgeben. Während der Lieferzeit
läuft Phase 1 und 3 ohnehin schon am PC — die sind fertig. Wenn du nicht warten
willst, den zweiten BMP581 bei Eckstein dazunehmen.

---

## Was nicht kaufen

| Falle | Warum |
|---|---|
| **BMP280 / BME280 / BMP180** | Viel zu langsam. Der Messbereich bis 100 Hz braucht ≥ 400 Hz Abtastrate, diese Sensoren schaffen einen Bruchteil davon. |
| **BMP390 als Ersatz** | 200 Hz Abtastrate. Die Firmware lehnt damit beide Messbänder ab: obere Bandgrenze darf höchstens 0,4 × Abtastrate sein, also 80 Hz. Nur als Zweitsensor für den Quervergleich sinnvoll. |
| **PLA-Filament** | Verformt sich ab ~55 °C. Hinter der Scheibe werden im Sommer 60–80 °C erreicht. |
| **Messmikrofon jeder Art** | Clippt ab ~130 dB und ist unter 20 Hz taub. Begründung in [01-messprinzip.md](01-messprinzip.md). |
| **Gore-Tex- oder Gewebemembran vor dem Druckport** | Strömungswiderstand verfälscht genau die tiefen Frequenzen. Nur offenporiger Schaumstoff. |
| **CYD-Varianten S032 / S035** | Andere Displays und Pins als in `config.h` hinterlegt. |
