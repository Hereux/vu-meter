# 2. Hardware und Stückliste

## Architektur

```
 12 V Bordnetz
   |  Sicherung 1 A + Verpolschutz + TVS (SMBJ24A)
   v
 DC/DC 12 V -> 5 V (MP1584 / LM2596)
   |
   v
 ESP32-2432S028R  ("Cheap Yellow Display")
   +-- 2.8" TFT 320x240 ILI9341 (SPI, onboard)
   +-- microSD (SPI, onboard)  -> Logging
   +-- I2C (GPIO 27 SDA / 22 SCL)              --> BMP581
   +-- Taster (GPIO 35, ext. Pull-Up 10k)      --> Mode / Peak-Reset
   +-- WLAN (optional)                          --> Web-UI / OTA
```

## Stückliste

| # | Teil | Zweck | Preis (EUR) | Quelle |
|---|---|---|---|---|
| 1 | ESP32-2432S028R, 2.8" TFT + microSD + Touch | MCU + Display + Logging in einem | 12-16 | AliExpress, Amazon |
| 2 | BMP581 **Breakout-Platine** (SparkFun Qwiic / DFRobot / generisch) | Messwandler | 12-29 | AliExpress ~12-15, Eckstein/BerryBase ~27-29. **Kein nackter LGA-10-Chip** |
| 3 | DC/DC 12 V -> 5 V, **fester** Ausgang, Eingang bis >= 40 V | Bordnetzversorgung | 3-7 | AliExpress ~3, oder RECOM R-78B50-10 (Reichelt Art. 159150) ~7 |
| 4 | TVS SMBJ24A, Schottky SS34, Sicherungshalter + 1 A Flachsicherung | Load-Dump-/Verpolschutz | 3-5 | Reichelt |
| 5 | Elkos 470 µF/35 V + 100 µF/10 V, Ferritperle | Siebung, Störunterdrückung | 2 | Reichelt |
| 6 | Kabel 2×0,75 mm², JST-XH-Steckverbinder, Schrumpfschlauch | Verkabelung | 4 | — |
| 7 | Filament ASA oder PETG, ~80 g | Gehäuse (hitzefest im Auto) | 2-3 | — |
| 8 | Gewindeeinsätze M3 (8×), Schrauben M3×8 | Gehäuseverschraubung | 3 | — |
| 9 | Saugnapfhalterung 30 mm (2×) oder GoPro-Mount | Befestigung Scheibe/Armaturenbrett | 4-7 | — |
| 10 | Taster 12 mm wasserdicht (optional, sonst Touch) | Peak-Reset | 2 | — |
| | **Summe** | | **55-95** | |

Optional im Budgetrahmen:

| Teil | Zweck | Preis |
|---|---|---|
| Zweiter Sensor BMP390 | Plausibilitätsprüfung / Redundanz | 8-12 |
| Kalibrier-Set: Schlauch 6 mm, 60 ml Spritze, Lineal | statische Gain-Prüfung (siehe Doc 05) | 5 |
| Bluetooth-/WLAN-Logging (nur Software) | Auswertung am Handy | 0 |

**Gesamt realistisch 55 EUR (AliExpress-Weg) bis 95 EUR (deutscher Weg)**
inkl. Kalibrierhilfsmittel — beides im Budgetrahmen.

Konkrete Bezugsquellen mit Artikelnummern: [08-einkaufsliste.md](08-einkaufsliste.md).

## Warum das ESP32-2432S028R-Board

Es ist die mit Abstand kosteneffektivste Option: ESP32-WROOM-32, 2,8" TFT,
Touch, microSD, USB-Seriell, Spannungsregler und LDR schon verbaut, für den
Preis eines nackten DevKits. Die klassische ESP32 leistet eine 512-Punkt-FFT bei
600 Hz Abtastrate mit <2 % CPU-Last — mehr als ausreichend.

Falls ein größerer Rechenpuffer oder besseres Sonnenlichtverhalten gewünscht
ist:
- **ESP32-S3 DevKitC-1 N16R8** (~14 EUR) + separates Display.
- **1,3" SH1106 OLED** (~7 EUR) statt TFT: bessere Ablesbarkeit bei Sonne, aber
  128×64 Pixel — Spektrumanzeige nur grob. Entspricht dem kommerziellen BHG-DB.

## Pinbelegung (ESP32-2432S028R)

| Signal | GPIO | Hinweis |
|---|---|---|
| I²C SDA | 27 | **nicht 21** — dort liegt die Beleuchtung, siehe unten |
| I²C SCL | 22 | |
| Hintergrundbeleuchtung | 21 | aktiv HIGH, per PWM geregelt |
| BMP581 VDD | 3V3 | max. 3,6 V, nicht an 5 V |
| Taster | 35 | Öffner (NC), **externer Pull-Up 10 kΩ gegen 3V3 nötig** |

> Pinbelegung vor dem Löten gegen die konkrete Boardrevision prüfen — bei den
> CYD-Boards gibt es mehrere Varianten. Frei sind üblicherweise GPIO 22, 27 und
> 35 (nur Eingang); TFT, Touch, SD, RGB-LED, LDR und Lautsprecher belegen den
> Rest.

**GPIO 21 ist die Hintergrundbeleuchtung, nicht frei.** Das ist die
Stolperfalle des Boards: 21/22 sind die Arduino-Standardpins für I²C, und wer
sie benutzt, bekommt ein funktionierendes Display mit dunkler Beleuchtung —
`Wire.begin()` konfiguriert GPIO 21 um und schaltet das Licht aus. Genau das
ist hier einmal passiert. `firmware/include/config.h` fängt es seitdem zur
Übersetzungszeit mit einem `static_assert` ab.

**Der Taster liegt auf GPIO 35, nicht auf GPIO 0.** Zwei Gründe:

1. **GPIO 0 ist ein Strapping-Pin.** Liegt er beim Reset auf Masse, startet
   der ESP32 in den Bootloader statt in die Anwendung. Bei einem seitlichen
   Taster heißt das: wer beim Einschalten drückt, hat ein scheinbar totes
   Gerät. An Zündungsplus im Fahrzeug passiert das früher oder später.
2. **GPIO 0 ist nicht herausgeführt.** Der Boot-Taster sitzt auf der
   Platine; ein externer Taster müsste an dessen Pads gelötet werden.

Dafür entfällt der Data-Ready-Interrupt des BMP581, der vorher auf 35
vorgesehen war. Gebraucht wurde er nie — die FIFO wird alle 20 ms gepollt
und läuft bei den gemessenen 494 Hz nie über.

> **GPIO 34–39 haben keine internen Pull-Widerstände.** `INPUT_PULLUP` wird
> dort kommentarlos ignoriert, der Eingang schwebt und liefert Zufallswerte —
> die Anzeige blättert dann scheinbar von selbst durch die Schirme. Der
> Taster braucht deshalb einen **externen 10 kΩ gegen 3V3**. Die Firmware
> prüft das beim Start und meldet es über die serielle Schnittstelle.

### Touchbedienung

Der ganze Schirm ist die Schaltfläche: tippen = Seite weiter, halten =
Haltewerte bzw. Referenz zurücksetzen. Keine gezeichneten Knöpfe, deshalb
auch keine Touchkalibrierung — gebraucht wird nur "wird berührt".

Der XPT2046 hängt auf dem CYD an einem **anderen SPI-Bus als das Display**.
TFT_eSPI benutzt ohne `USE_HSPI_PORT` das globale `SPI`-Objekt, und das ist
**VSPI**. Der Touchtreiber muss deshalb auf **HSPI** liegen; beide auf
demselben Peripheriegerät heißt, dass der zuletzt aufgerufene `begin()` die
Pins umlegt und der andere ins Leere sendet. Genau daran ist es beim ersten
Versuch gescheitert.

| Signal | GPIO |
|---|---|
| T_CLK | 25 |
| T_DO (MISO) | 39 |
| T_DIN (MOSI) | 32 |
| T_CS | 33 |

**Am Prototyp gemessen**, Ruhezustand: `z1 = 0`, `z2 = 4084–4094`, daraus
Druckwert **11**. Gegen die Schwelle von 400 ist das Faktor 36 Abstand —
da löst nichts versehentlich aus. Sollte das Tippen einmal zu fest geraten
müssen, ließe sich `kTouchThreshold` bis etwa 150 senken, ohne dem Rauschen
nahe zu kommen.

### Öffner oder Schließer

Verbaut ist ein **Öffner (NC)**. Mit dem Pull-Up gegen 3V3 und dem Taster
gegen Masse kehren sich damit die Pegel um:

| Tastertyp | Ruhe | gedrückt |
|---|---|---|
| Schließer (NO) | offen → **HIGH** | zu → **LOW** |
| Öffner (NC) | zu → **LOW** | offen → **HIGH** |

In der Firmware hängt das an einer einzigen Zeile, `kButtonNormallyClosed`
in `firmware/include/config.h`. Steht die falsch, läuft die Bedienung
spiegelverkehrt: das Gerät blättert im Ruhezustand dauernd weiter und
reagiert beim Drücken nicht. Beim Tauschen des Tasters ist es die einzige
Stelle, die sich ändert.

> **Die Startprüfung kann beim Öffner einen fehlenden Pull-Up nicht sehen.**
> Der Öffner ist im Ruhezustand geschlossen und zieht den Pin hart auf
> Masse — mit oder ohne Widerstand. Der fehlende Pull-Up fällt erst beim
> Drücken auf, wenn der Kontakt öffnet und der Eingang schwebt. Symptom:
> ein Taster, der scheinbar wahllos mal reagiert und mal nicht. Beim
> Schließer deckt die Prüfung den Fall dagegen ab.

Nebeneffekt des Öffners: im Ruhezustand fließt dauerhaft Strom über den
Pull-Up, 3,3 V / 10 kΩ = 0,33 mA. Gegen die rund 150 mA von ESP32 und
Display fällt das nicht ins Gewicht. Ein hoch­ohmigerer Widerstand wäre an
der langen Tasterleitung deutlich störempfindlicher, deshalb bleibt es
bei 10 kΩ.

Dazu **100 nF parallel zum Taster**. Das entprellt und ist in einem Fahrzeug
mit 1000-W-Endstufe vor allem Störschutz: die Leitung zum seitlichen Taster
ist eine Antenne. Mit dem 10 k ergibt sich 1 ms Zeitkonstante — unauffällig
gegenüber der 40-ms-Entprellung in der Firmware.

```
3V3 ---[10k]---+--- GPIO 35
               |
               +---[100n]---+
               |            |
          Taster (NC)       |
               |            |
GND -----------+------------+

      Ruhe: Kontakt zu  -> GPIO 35 = LOW
  gedrueckt: Kontakt auf -> GPIO 35 = HIGH (ueber den Pull-Up)
```

I²C mit 400 kHz. Bei 32-Byte-FIFO-Burst alle ~50 ms ist die Buslast minimal.
Kabel zum Sensor kurz halten (<30 cm) oder auf SPI wechseln.

## Elektrische Auslegung Bordnetz

Ein Showcar-Bordnetz ist rau: 9-18 V im Normalbetrieb, Load-Dump-Spitzen,
Ripple von der Endstufe.

1. Flachsicherung 1 A direkt an der Einspeisung.
2. Verpolschutz: Schottky SS34 in Reihe (0,4 V Verlust, unkritisch).
3. TVS SMBJ24A parallel gegen Spitzen.
4. 470 µF/35 V vor dem Wandler, Ferritperle + 100 µF nach dem Wandler.
5. Masse **sternförmig** an einem Punkt, nicht an der Endstufenmasse — sonst
   koppelt der Ripple als Störsignal ein.

Der Drucksensor ist immun gegen elektromagnetische Einstreuung der Endstufe
(digitaler Bus, kein analoges Mikrofonsignal) — ein weiterer Vorteil gegenüber
Mikrofonlösungen.

## Mechanische Anforderungen an den Sensor

- Der Sensorport muss **offen zum Fahrzeuginnenraum** liegen, kein geschlossenes
  Gehäusevolumen. Ein abgedichtetes Gehäuse wirkt als Hochpass und verfälscht
  gerade unter 20 Hz massiv.
- Gleichzeitig Schutz vor direktem Luftzug und Staub: Port mit Schaumstoff
  (offenporig, ~5 mm) abdecken. Der ist bei diesen Frequenzen akustisch
  transparent.
- Sensorplatine mechanisch entkoppeln (Moosgummi), sonst misst man
  Gehäuseresonanzen mit.
