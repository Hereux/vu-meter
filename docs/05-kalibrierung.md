# 5. Kalibrierung und Verifikation

Das Gerät ist durch das Messprinzip bereits absolut kalibriert (der Sensor
liefert Pa). Zu prüfen sind deshalb nur drei Dinge: **Gain**, **Frequenzgang**
und **Rauschteppich**.

## A. Gain-Prüfung mit der Wassersäule (statisch)

### Was hier eigentlich passiert

Der BMP581 misst den Druck der Luft, die an seinem Port anliegt — sonst nichts.
Um ihm einen *bekannten* Druck vorzusetzen, muss man ihn also **in ein dichtes
Volumen einsperren** und den Druck in genau diesem Volumen um einen bekannten
Betrag anheben. Ein Schlauch, der irgendwo danebenliegt, tut gar nichts.

Das Anheben übernimmt die Wassersäule. Ein U-Rohr-Manometer ist eine Waage:
steht das Wasser in den beiden Schenkeln um Δh versetzt, dann ist der Druck auf
der geschlossenen Seite genau `p_umgebung + ρ · g · Δh` — unabhängig von
Schlauchdurchmesser, Wassermenge und Volumen der Kammer. Nur die
Höhendifferenz zählt. Deshalb funktioniert die Methode mit Baumarktmaterial.

### Aufbau

```
      Spritze 60 ml
       (drückt Luft nach)
            |
            v
      +-----------+
      | T-Stück   |------ Schlauch -----> Sensorkammer (dicht, Sensor drin)
      +-----------+                        - hoch aufhaengen! -
            |
         Schlauch
            |
            v
      +--------------------+
      |   U-Rohr, Wasser   |   offenes Ende zur Raumluft
      |   Delta h ablesen  |
      +--------------------+
```

**Sensorkammer:** ein kleines Schraubglas oder ein Stück 20-mm-Rohr mit zwei
Kappen. Zwei Durchführungen: eine Schlauchtülle für den Schlauch, eine Bohrung
für das Sensorkabel. Beide mit Heißkleber oder Epoxid abdichten. Das Volumen
ist egal — nur die Dichtheit zählt.

**Die Kammer gehört nach oben, das U-Rohr nach unten**, mit reichlich Schlauch
dazwischen. Wasser im Sensor ist das Ende des Sensors.

### Ablauf

1. Alles drucklos. Am Gerät den **Rohdruck** ablesen → `p0`.
   Das ist der Wert `rawPressurePa` aus dem Report; er liegt vor dem
   DC-Blocker, weil genau der sonst die zu messende Größe entfernen würde.
2. Spritze langsam eindrücken, bis das Wasser um Δh versetzt steht.
3. **30 s warten.** Komprimierte Luft erwärmt sich; der Druck sinkt danach
   noch etwas, bis die Temperatur wieder stimmt. Der BMP581 liefert die
   Temperatur mit, daran lässt sich das Einschwingen beobachten.
4. Δh mit dem Stahllineal ablesen, gleichzeitig den Rohdruck → `p1`.
5. Prüfen: `p1 − p0` muss `ρ · g · Δh` entsprechen.

Das für mehrere Höhen wiederholen — 10, 20, 50 cm. Ein einzelner Punkt zeigt
nur einen Offset; erst die Reihe zeigt, dass der Gain über den Bereich stimmt.

### Sollwerte (Wasser bei 20 °C, ρ = 998,2 kg/m³, g = 9,80665 m/s²)

| Δh | Überdruck | als Effektivwert | Ablesefehler bei ±1 mm |
|---|---|---|---|
| 5 cm | 489 Pa | 147,8 dB | ±2,0 % |
| 10 cm | 979 Pa | 153,8 dB | ±1,0 % |
| 20 cm | 1958 Pa | 159,8 dB | ±0,5 % |
| **50 cm** | **4896 Pa** | **167,8 dB** | **±0,2 %** |

**Nimm die 50 cm als Hauptmesspunkt.** Bei 10 cm ist dein Lineal der
begrenzende Faktor, nicht der Sensor. Bei 50 cm liegt der Ablesefehler mit
0,2 % gleichauf mit der Temperaturabhängigkeit des Wassers (15 → 25 °C sind
ebenfalls 0,2 %) — zusammen etwa 0,3 %. Genauer wird diese Methode nicht, und
genauer muss sie auch nicht sein.

Die dB-Spalte ist reine Umrechnung zur Einordnung: 4896 Pa sind der
Effektivdruck, den ein 167,8-dB-Ton hätte. Es ist ein statischer Druck, kein
Schall — aber in derselben Größenordnung wie das, was du später misst.

**Prüfkriterium: Abweichung < 1 %.** Damit ist die Pa-Skala des Sensors über
den gesamten Messbereich bestätigt, mit Hausmitteln und ohne Referenzgerät.
Das kann kein Mikrofonaufbau.

### Lecks erkennen

Ein Leck verrät sich von selbst: Rohdruck und Wasserstand sinken gemeinsam und
stetig. Bleiben beide über eine Minute stabil, ist der Aufbau dicht. Sinkt nur
der Druck, ohne dass sich das Wasser bewegt — dann misst du falsch, prüf die
Verbindung zur Kammer.

### Sofort-Check ohne jeden Aufbau

Bevor du irgendetwas baust: Der Luftdruck fällt mit der Höhe um **rund 12 Pa
pro Meter**. Trag das Gerät drei Stockwerke hoch, das sind etwa 10 m, und der
Rohdruck muss um ~120 Pa fallen. Kostet nichts, dauert zwei Minuten und deckt
grobe Fehler sofort auf: falscher Sensor, falsche Einheit, kaputte Skalierung.

### Was diese Prüfung nicht abdeckt

Sie prüft den **Gain bei 0 Hz**. Sie sagt nichts über den Frequenzgang — dafür
ist Abschnitt B da. Und sie prüft nicht die Signalkette dahinter (Bandfilter,
FFT, Detektoren); die ist bereits durch die 151 Tests in
`firmware/test/test_dsp/` abgedeckt und braucht keine Hardware.

## B. Frequenzgang mit DIY-Pistonphon (dynamisch)

Bekanntes Volumen V, Lautsprecher verschiebt ΔV:
`Δp = γ · P₀ · ΔV / V` (γ = 1,4 adiabat)

Aufbau: Marmeladenglas o. ä. mit bekanntem Volumen, Deckel mit kleinem
Lautsprecher dicht eingeklebt, Sensorport dicht eingeführt. Speaker mit Sinus
10-150 Hz ansteuern.

Der absolute Wert ist damit nur grob bestimmbar (ΔV ist schwer zu messen),
**der relative Frequenzgang aber sehr gut** — und genau den braucht man, um die
Sinc-Korrektur und eventuelle Kammerresonanzen zu bestätigen. Erwartung: flach
±0,5 dB von 10 bis 100 Hz.

Fallback ohne Pistonphon: Zwei Sensoren unterschiedlichen Typs (BMP581 +
BMP390) parallel im Fahrzeug messen lassen. Systematische Abweichungen zeigen
sich als Differenz über der Frequenz.

## C. Rauschteppich

Gerät in einem ruhigen Raum betreiben, 60 s aufzeichnen, SPL im Band 10-100 Hz
auswerten. Erwartung: 85-100 dB, abhängig vom Oversampling. Das ist der
untere Rand des Messbereichs — für Bassmessungen ab 120 dB völlig ausreichend,
aber im Datenblatt des eigenen Geräts ehrlich anzugeben.

## D. Vergleich im Fahrzeug

Gegen ein vorhandenes Gerät (TermLAB, BHG-DB, oder das Gerät eines Kollegen auf
einem Treffen) an identischer Position gegenmessen. Erwartete Abweichung bei
sauberer Umsetzung: wenige dB — Restdifferenzen kommen fast immer von
unterschiedlicher Sensorposition, nicht vom Gerät.

## E. Was das Gerät NICHT kann

Ehrlichkeitsabschnitt für die Doku:

- Kein geeichtes Messgerät, keine Klasse-1/2-Zertifizierung nach IEC 61672.
- Keine offiziellen Wettbewerbsergebnisse — dafür gelten dort vorgeschriebene
  Geräte und Mikrofonpositionen.
- Oberhalb ~150 Hz nicht ausgelegt (Anti-Aliasing, Sinc-Korrektur).
- Absolutwerte hängen stark von der Messposition im Fahrzeug ab.

## Dokumentation der Kalibrierung

Ergebnisse in `docs/kalibrierprotokoll.md` festhalten: Datum, Seriennummer des
Sensors, gemessene fs, Wassersäulen-Abweichung in %, Rauschteppich, ggf.
eingetragener Korrekturoffset in dB.
