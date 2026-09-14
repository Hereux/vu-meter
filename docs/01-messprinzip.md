# 1. Messprinzip

## Warum kein Mikrofon

| Anforderung | Elektret / MEMS-Mikrofon | Absolut-Drucksensor (Barometer) |
|---|---|---|
| Untere Grenzfrequenz | typ. 20-100 Hz (AC-gekoppelt), 10 Hz praktisch nicht | DC bis >150 Hz, kein Hochpass |
| Max. Pegel (AOP) | 130-135 dB, danach Klirr/Clipping | ~178 dB (siehe unten) |
| Kalibrierung | Empfindlichkeit in mV/Pa, driftet, braucht Kalibrator | liefert direkt Pa, werkseitig gainkalibriert |
| Kosten | 2-15 EUR | 8-20 EUR |

Bei 1000 W Sub im Fahrzeug werden 150-160 dB erreicht. Jedes bezahlbare Mikrofon
ist dort weit im Clipping. Deshalb: **Absolut-Drucksensor als Messwandler.**

Kommerzielle Geräte dieser Klasse (z. B. Basshead Garage BHG-DB: 120-180 dB,
10-100 Hz) arbeiten nach demselben Prinzip.

## Physik

Schalldruckpegel:  `L = 20 · log10(p_rms / 20 µPa)`

| dB SPL | p_rms | p_peak |
|---|---|---|
| 120 | 20 Pa | 28 Pa |
| 140 | 200 Pa | 283 Pa |
| 150 | 632 Pa | 894 Pa |
| **155** | **1125 Pa** | **1591 Pa** (16 hPa) |
| 160 | 2000 Pa | 2828 Pa |
| 180 | 20000 Pa | 283 hPa |

Ein Barometer misst genau diese Größe direkt in Pa. Der statische Luftdruck
(~1013 hPa) wird digital weggefiltert, übrig bleibt der Wechselanteil = Schall.

**Kernvorteil:** Pa sind Pa. Es gibt keine unbekannte Mikrofonempfindlichkeit,
die kalibriert werden müsste. Die Absolutgenauigkeit hängt nur am Gain-Fehler
des Sensors (Größenordnung ±0,5 %, also ±0,05 dB) plus der Frequenzgang-
korrektur. Das ist deutlich besser als jedes selbstgebaute Mikrofonsetup.

## Sensorwahl

**Primär: Bosch BMP581**

- Bereich 30-125 kPa (300-1250 hPa)
- Auflösung 1/64 Pa (≈ 58 dB SPL rechnerische Untergrenze)
- Rauschen 0,08 Pa bei hohem Oversampling; bei OSR 1x / ~600 Hz ODR grob 1-2 Pa
  → Rauschteppich breitbandig ≈ 95-100 dB SPL, bandbegrenzt auf 10-100 Hz deutlich darunter
- ODR bis 622 Hz im Continuous Mode (SparkFun misst real ~500 Hz), FIFO 32 Werte
- I²C/SPI, 1,71-3,6 V

Aussteuerung: bei 1013 hPa Umgebungsdruck bleiben 237 hPa bis zur Obergrenze
→ **Clipping-Grenze ≈ 178 dB SPL**. Zielwert 155 dB liegt 23 dB darunter.
Nutzbare Dynamik damit rund 95 … 178 dB.

**Alternativen** (falls BMP581 nicht lieferbar):

| Sensor | ODR max | Rauschen | Bemerkung |
|---|---|---|---|
| BMP390 / BMP388 | 200 Hz | 0,02-0,9 Pa | reicht für 10-80 Hz, günstiger |
| TDK ICP-20100 | 400 Hz | 0,4 Pa | FIFO, sehr rauscharm |
| Infineon DPS310 | 128 Hz | 0,34 Pa | Nyquist nur 64 Hz, grenzwertig |

Faustregel: ODR ≥ 4 × höchste Messfrequenz. Für 10-100 Hz also ≥ 400 Hz.

## Abtastung und Aliasing

Der Sensor hat keinen analogen Anti-Aliasing-Filter. Sein ADC mittelt aber über
das Wandlungsintervall, das wirkt als sinc-Tiefpass mit Nullstelle bei fs.
Musikinhalt oberhalb 300 Hz (fs/2 bei 622 Hz) faltet sich theoretisch zurück,
liegt aber im Fahrzeug beim Basspegel typ. 30-50 dB unter dem Subbass — und der
digitale Bandpass 5-150 Hz entfernt, was nicht zurückfaltet. Für die
Zielanwendung (Bassmessung) unkritisch; im Dokument `05-kalibrierung.md` wird
das messtechnisch verifiziert.

Sinc-Korrektur, die in der Firmware auf den Betragsgang addiert wird:

| Frequenz | fs = 500 Hz | fs = 622 Hz |
|---|---|---|
| 10 Hz | +0,01 dB | +0,00 dB |
| 50 Hz | +0,14 dB | +0,09 dB |
| 100 Hz | +0,58 dB | +0,37 dB |
| 150 Hz | +1,33 dB | +0,85 dB |

Nachrechnen: `python3 tools/spl_calc.py`
