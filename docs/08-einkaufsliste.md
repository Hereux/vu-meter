# 8. Einkaufsliste — AliExpress zuerst

> **Was geprüft ist und was nicht.** Die Shop-Seiten sind aus der
> Arbeitsumgebung nicht abrufbar — das Gateway lehnt `aliexpress.com`,
> `amazon.de` und `reichelt.de` mit HTTP 403 ab, das ist die Netzwerkrichtlinie
> der Umgebung. Die Artikelnummern unten stammen aus **Suchtreffern**, die
> Seiten selbst wurden **nicht geöffnet**. Preise, Lagerstand und die genaue
> Produktidentität also vor dem Kauf am Bild prüfen — besonders bei Punkt 2,
> dort gibt es eine teure Verwechslungsgefahr.
>
> Die Artikelnummer ist bei AliExpress das Dauerhafte: wenn ein Link ins Leere
> läuft, die Nummer in die Suche der App eintippen.

## Eine Bestellung, alles von AliExpress

| # | Teil | Artikelnummer | ca. |
|---|---|---|---|
| 1 | ESP32-2432S028R (CYD) | 1005004700627096 | 13 € |
| 2 | BMP581 **Breakout-Platine** | 1005005030982017 | 15 € |
| 3 | Step-Down 12 V → 5 V, **fest** | Suche, s.u. | 3 € |
| 4 | KFZ-Sicherungshalter + Sicherungen | Suche | 2 € |
| 5 | TVS-Dioden-Sortiment SMBJ/P6KE | Suche | 2 € |
| 6 | JST-XH-Set + Litze 0,75 mm² | Suche | 4 € |
| 7 | Gewindeeinsätze M3 (Set) | Suche | 4 € |
| 8 | Saugnapfhalterung 30 mm, 2× | Suche | 3 € |
| 9 | Mikrofon-Windschutz Schaumstoff | Suche | 2 € |
| 10 | Moosgummi 2 mm selbstklebend | Suche | 3 € |
| 11 | Silikonschlauch 6 mm klar + Spritze 60 ml | Suche | 4 € |
| | **Summe AliExpress** | | **~55 €** |

Dazu **Filament** (ASA oder PETG, ~25 € die Rolle) aus einem deutschen Shop —
Versandkosten und Zoll machen eine Filamentrolle aus China unattraktiv, und du
hast vermutlich ohnehin welches da.

---

## 1. ESP32-2432S028R — Rechner und Display

Mehrere bestätigte Listings, alle dasselbe Board:

| Artikelnummer | Bezeichnung im Treffer |
|---|---|
| [1005004700627096](https://de.aliexpress.com/item/1005004700627096.html) | RCmall 2.8'' ESP32-2432S028R, 240×320, ILI9341 |
| [1005004502250619](https://de.aliexpress.com/item/1005004502250619.html) | ESP32 LVGL WiFi/BT 2.8" Smart Display, im Treffer mit 9,39 US$ |
| [1005004971720824](https://de.aliexpress.com/item/1005004971720824.html) | 2.8 Inch Display Screen ESP32 LVGL WiFi BT |

**Vor dem Kaufen am Bild prüfen:** gelbe Platine, Typaufdruck
`ESP32-2432S028R`, resistiver Touch (dünner Stift liegt oft bei), microSD-Slot
an der Seite. Die Varianten `S032` und `S035` haben andere Displays und andere
Pins als in `firmware/include/config.h` hinterlegt.

Pinout zum Gegenprüfen nach Erhalt:
[Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r/) ·
[Mischianti](https://mischianti.org/esp32-2432s028-cheap-yellow-display-high-resolution-pinout-datasheet-schema-and-specs/)

---

## 2. BMP581 — hier ist die Falle

**Die Hälfte der „BMP581"-Angebote sind nackte Chips im LGA-10-Gehäuse:
2 × 2 × 0,5 mm, zehn Pads unter dem Bauteil.** Von Hand nicht lötbar, ohne
Platine nutzlos. Die sind billig, und genau deshalb stehen sie oben in den
Suchergebnissen.

### Das willst du — Breakout-Platinen

| Artikelnummer | Was | ca. |
|---|---|---|
| [1005005030982017](https://de.aliexpress.com/item/1005005030982017.html) | SparkFun SEN-20170 Qwiic BMP581 (Nachbau o. Reseller) | ~15 € |
| [1005011832116348](https://de.aliexpress.com/item/1005011832116348.html) | DFRobot Gravity BMP581, I2C/UART | ~14 € |
| [1005011635538614](https://de.aliexpress.com/item/1005011635538614.html) | DFRobot Fermion BMP581, I2C/SPI/I3C, 3,3 V | ~12 € |
| [1005010277584150](https://de.aliexpress.com/item/1005010277584150.html) | Adafruit-6407-Variante, STEMMA QT | ~13 € |
| [1005010701836930](https://de.aliexpress.com/item/1005010701836930.html) | „BMP581 High Precision Barometric" — Titel lässt offen, ob Platine. **Bilder prüfen.** | ~10 € |

> Die letzten drei Nummern sind aus Treffern anderer AliExpress-Länderdomains
> umgerechnet (dort `3256 8…`, hier `1005 0…` mit identischen letzten zehn
> Ziffern). Das Muster stimmt normalerweise, überprüft habe ich es nicht.

### Das willst du nicht — nackte Chips

[1005008158262909](https://de.aliexpress.com/item/1005008158262909.html)
(„BMP581 LGA-10 (2x2x0.5)") und
[1005008147941143](https://de.aliexpress.com/item/1005008147941143.html)
(„2PCS/LOT New original BMP581"). Erkennungsmerkmal im Bild: ein winziges
schwarzes Quadrat, teils auf Klebeband oder im Gurt, **ohne Platine und ohne
Stiftleiste**.

**Kaufempfehlung: zwei Stück, verschiedene Anbieter.** 28 € für zwei sind gut
angelegt — der zweite ist die Plausibilitätsprüfung aus Phase 5.3, und wenn
einer defekt oder als Chip statt Platine ankommt, steht das Projekt nicht noch
einmal vier Wochen.

**Anschluss: 3,3 V, niemals 5 V.** Laut Datenblatt maximal 3,6 V.

Wenn du nicht warten willst oder auf Nummer sicher gehen möchtest, gibt es
dasselbe in Deutschland ab Lager:
[Eckstein](https://eckstein-shop.de/SparkFun-Pressure-Sensor-BMP581-Qwiic-Compatible-with-Arduino-EN)
27,31 € · [BerryBase](https://www.berrybase.at/sparkfun-qwiic-bmp581) ~28 €.

---

## 3. Spannungsversorgung — zwei Regeln

[Suche: DC-DC 12V 24V auf 5V Step-Down](https://de.aliexpress.com/w/wholesale-dc--dc-12v-24v-to-5v-step-down-module.html)

**Regel 1: feste 5 V, kein einstellbares Modul.** Module mit Trimmpoti sind
billiger und überall zu haben — aber ein Poti in einem Fahrzeug, das von einem
1000-W-Sub durchgeschüttelt wird, kann sich verstellen. Dann liegen 12 V am
ESP32 an und das Board ist hin. Wenn es doch ein einstellbares wird: nach dem
Einstellen einen Tropfen Nagellack oder Sicherungslack aufs Poti.

**Regel 2: Eingangsbereich bis mindestens 40 V.** Load-Dump-Spitzen im Bordnetz
gehen weit über 14 V hinaus. Module mit „bis 28 V" sind zu knapp.

Zum XL4015, der oft empfohlen wird: in den Suchergebnissen finden sich
[Hinweise auf Fälschungen](https://de.aliexpress.com/w/wholesale-xl4015.html),
bei denen ein LM2596 unter dem Aufdruck sitzt und das Modul schon bei kleiner
Last heiß wird. Deshalb lieber ein fertiges KFZ-Modul mit festem 5-V-Ausgang.

**Wenn du Gewissheit willst**, ist das der eine Punkt, an dem sich eine
Reichelt-Position lohnt: **RECOM R-78B50-10**, Artikelnummer **159150**, ~7 €.
6,5–32 V Eingang, feste 5 V, 1 A, drei Pins. Damit ist die Versorgung erledigt.

**Dazu unbedingt, egal woher:**

| Teil | AliExpress-Suche |
|---|---|
| KFZ-Flachsicherungshalter + 1-A-Sicherungen | [Suche](https://de.aliexpress.com/w/wholesale-inline-blade-fuse-holder-car.html) |
| TVS-Dioden-Sortiment (SMBJ oder P6KE, 24 V) | [Suche](https://de.aliexpress.com/w/wholesale-p6ke-tvs-diode-kit.html) |
| Schottky-Dioden SS34 / 1N5822 (Verpolschutz) | [Suche](https://de.aliexpress.com/w/wholesale-ss34-schottky-diode.html) |
| Elkos 470 µF/35 V und 100 µF/16 V | [Suche](https://de.aliexpress.com/w/wholesale-electrolytic-capacitor-kit.html) |

Sicherung direkt an die Einspeisung, Schottky in Reihe, TVS parallel.
Masse sternförmig an **einem** Punkt, nicht an der Endstufenmasse — sonst
koppelt deren Ripple ein.

---

## 4. Mechanik

| Teil | Suche |
|---|---|
| Gewindeeinsätze M3 zum Einschmelzen | [Suche](https://de.aliexpress.com/w/wholesale-m3-heat-set-insert-brass.html) |
| Schrauben M3×8 Zylinderkopf | [Suche](https://de.aliexpress.com/w/wholesale-m3-socket-head-screw-kit.html) |
| Saugnapfhalterung 30 mm | [Suche](https://de.aliexpress.com/w/wholesale-suction-cup-mount-car-windshield.html) |
| Mikrofon-Windschutz Schaumstoff | [Suche](https://de.aliexpress.com/w/wholesale-microphone-foam-windscreen.html) |
| Moosgummi 2 mm selbstklebend | [Suche](https://de.aliexpress.com/w/wholesale-self-adhesive-foam-rubber-sheet-2mm.html) |

**Filament aus Deutschland**, ASA oder PETG. Kein PLA — hinter der Scheibe
werden im Sommer 60–80 °C erreicht, PLA verformt sich ab 55 °C.

---

## 5. Kalibrierhilfsmittel

| Teil | Suche | Zweck |
|---|---|---|
| Silikonschlauch 6 mm klar, 1 m | [Suche](https://de.aliexpress.com/w/wholesale-clear-silicone-tube-6mm.html) | U-Rohr-Manometer |
| Einwegspritze 60 ml | [Suche](https://de.aliexpress.com/w/wholesale-60ml-syringe.html) | DIY-Pistonphon |

Damit wird die Gain-Prüfung bis 168 dB möglich — ohne Referenzgerät.
10 cm Wassersäule sind exakt 980 Pa, also 153,8 dB. Details in
[05-kalibrierung.md](05-kalibrierung.md).

---

## Was du bewusst nicht kaufst

| Falle | Warum |
|---|---|
| **BMP581 als nackter LGA-10-Chip** | 2 × 2 mm, Pads unter dem Bauteil. Von Hand nicht lötbar. |
| **BMP280 / BME280 / BMP180** | Viel zu langsam. 100 Hz Messbereich braucht ≥ 400 Hz Abtastrate. |
| **BMP390 als Ersatz** | 200 Hz. Die Firmware lehnt damit beide Messbänder ab — obere Grenze darf höchstens 0,4 × Abtastrate sein, also 80 Hz. Nur als Zweitsensor zum Quervergleich. |
| **Einstellbares Netzteilmodul im Auto** | Trimmpoti kann sich durch Vibration verstellen, dann liegen 12 V am ESP32. |
| **Netzteilmodul mit nur 28 V Eingang** | Zu knapp für Load-Dump-Spitzen. |
| **PLA-Filament** | Verformt sich ab 55 °C. |
| **Messmikrofon jeder Art** | Clippt ab ~130 dB, unter 20 Hz taub. Begründung in [01-messprinzip.md](01-messprinzip.md). |
| **Gore-Tex oder Gewebe vor dem Druckport** | Strömungswiderstand frisst genau die tiefen Frequenzen. Nur offenporiger Schaumstoff. |
| **CYD-Varianten S032 / S035** | Andere Displays und Pins als in `config.h`. |

---

## Reihenfolge

1. AliExpress-Bestellung raus — **CYD + zwei BMP581-Breakouts** sind der
   kritische Pfad, 2–4 Wochen.
2. Filament besorgen, falls nicht vorrätig.
3. Optional Reichelt-Position R-78B50-10 (Art. 159150), wenn du beim Netzteil
   kein Risiko willst.

Warten kostet keinen Fortschritt: Phase 1 und der PC-Teil von Phase 3 sind
fertig, die 142 Tests laufen ohne Hardware.
