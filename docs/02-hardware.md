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
   +-- I2C (GPIO 21 SDA / 22 SCL, Header CN1)  --> BMP581
   +-- Taster (GPIO 35 o. Touch)               --> Mode / Peak-Reset
   +-- WLAN (optional)                          --> Web-UI / OTA
```

## Stückliste

| # | Teil | Zweck | Preis (EUR) | Quelle |
|---|---|---|---|---|
| 1 | ESP32-2432S028R, 2.8" TFT + microSD + Touch | MCU + Display + Logging in einem | 12-16 | AliExpress, Amazon |
| 2 | BMP581 Breakout (SparkFun Qwiic / DFRobot Gravity / generisch) | Messwandler | 8-19 | SparkFun, DFRobot, AliExpress |
| 3 | DC/DC-Wandler 12 V -> 5 V, 2 A (MP1584 o. ä.) | Bordnetzversorgung | 2-4 | AliExpress |
| 4 | TVS SMBJ24A, Schottky SS34, Sicherungshalter + 1 A Flachsicherung | Load-Dump-/Verpolschutz | 3-5 | Reichelt |
| 5 | Elkos 470 µF/35 V + 100 µF/10 V, Ferritperle | Siebung, Störunterdrückung | 2 | Reichelt |
| 6 | Kabel 2×0,75 mm², JST-XH-Steckverbinder, Schrumpfschlauch | Verkabelung | 4 | — |
| 7 | Filament ASA oder PETG, ~80 g | Gehäuse (hitzefest im Auto) | 2-3 | — |
| 8 | Gewindeeinsätze M3 (8×), Schrauben M3×8 | Gehäuseverschraubung | 3 | — |
| 9 | Saugnapfhalterung 30 mm (2×) oder GoPro-Mount | Befestigung Scheibe/Armaturenbrett | 4-7 | — |
| 10 | Taster 12 mm wasserdicht (optional, sonst Touch) | Peak-Reset | 2 | — |
| | **Summe** | | **42-66** | |

Optional im Budgetrahmen:

| Teil | Zweck | Preis |
|---|---|---|
| Zweiter Sensor BMP390 | Plausibilitätsprüfung / Redundanz | 8-12 |
| Kalibrier-Set: Schlauch 6 mm, 60 ml Spritze, Lineal | statische Gain-Prüfung (siehe Doc 05) | 5 |
| Bluetooth-/WLAN-Logging (nur Software) | Auswertung am Handy | 0 |

**Gesamt realistisch 55-85 EUR** inkl. Kalibrierhilfsmittel — passt ins Budget.

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
| I²C SDA | 21 | Header CN1 |
| I²C SCL | 22 | Header CN1 |
| BMP581 VDD | 3V3 | max. 3,6 V, nicht an 5 V |
| BMP581 INT | 35 | optional, Data-Ready-Interrupt (nur Eingang!) |
| Taster | 0 / Touch | Peak-Reset |

> Pinbelegung vor dem Löten gegen die konkrete Boardrevision prüfen — bei den
> CYD-Boards gibt es mehrere Varianten. Frei sind üblicherweise GPIO 21, 22 und
> 35 (nur Eingang) an CN1/P3; TFT, Touch und SD belegen den Rest.

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
