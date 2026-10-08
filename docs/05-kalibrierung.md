# 5. Kalibrierung und Verifikation

Das Gerät ist durch das Messprinzip bereits absolut kalibriert (der Sensor
liefert Pa). Zu prüfen sind deshalb nur drei Dinge: **Gain**, **Frequenzgang**
und **Rauschteppich**.

## Stand

| Prüfung | belegt | |
|---|---|---|
| Stockwerktest (0 Hz) | Pa-Skala, umkehrbar, driftfrei | ✅ |
| Scheitelfaktor 3,0 dB | Wechselanteil-Kette vollständig | ✅ |
| Spritzentest, +3450 Pa | Messbereich bis ~165 dB, kein Überlauf | ✅ |
| Rauschteppich | 87 dB | ✅ |
| **Wassersäule** | — | **übersprungen, siehe A** |
| **Frequenzgang im Gehäuse** | — | **offen, Phase 6.5** |

## A. Gain-Prüfung mit der Wassersäule — **übersprungen**

Der Sensor liefert einen 24-Bit-Zähler, die Firmware multipliziert mit
1/64 Pa. Dieselbe Konstante, derselbe Rechenweg für 85 Pa wie für 1600 Pa —
dazwischen gibt es keine Messbereichsumschaltung und keine analoge
Verstärkung. **Ein Faktorfehler hätte den Stockwerktest verfehlt**, der auf
+7,0 m kam und auf exakt 0,0 m zurück. Die Wassersäule würde dieselbe
Multiplikation ein zweites Mal prüfen, nur genauer.

Dazu kam ein Spritzentest am Einmachglas: 995,5 → 1030 hPa, also +3450 Pa.
Rückgerechnet über die Gasgleichung ergibt das bei 24 ml Hub ein
Systemvolumen von 717 ml (isotherm) bis 998 ml (adiabat) — ein Weckglas mit
Schlauch liegt genau dort. Der Messwert ist also auch quantitativ mit der
Physik verträglich.

Was damit **nicht** geprüft ist, ist der akustische Weg: Gehäuse, Druckport,
Kammervolumen. Das ist eine andere Fehlerquelle und wird in Phase 6.5
geprüft, indem frei liegend gegen fertiges Gehäuse gemessen wird.

Die ursprüngliche Anleitung steht unten weiter, falls der Test doch einmal
gebraucht wird.

## A1. Anleitung Wassersäule (für den Bedarfsfall)

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

### Sofort-Check ohne jeden Aufbau — **bestanden**

Bevor du irgendetwas baust: Der Luftdruck fällt mit der Höhe um **rund 12 Pa
pro Meter**. Auf dem Kalibrierschirm die Referenz setzen, nach oben gehen, die
Differenz ablesen. Kostet nichts, dauert zwei Minuten und deckt grobe Fehler
sofort auf: falscher Sensor, falsche Einheit, kaputte Skalierung.

**Ergebnis am fertigen Gerät:**

| | |
|---|---|
| Referenz unten, höchster Punkt im Haus | −84,6 Pa → **+7,0 m** |
| zurück auf Referenzhöhe | **0,0 m** |
| Temperatur | 17,5 °C, plausibel |
| Absolutdruck | 1006,03 hPa |

Entscheidend ist nicht der Hinweg, sondern die Rückkehr auf **exakt 0,0 m**:
damit ist die Messung umkehrbar und driftfrei, nicht bloß eine Zahl, die in
eine Richtung läuft. Die Pa-Skala stimmt über den gesamten Signalweg vom
Sensorregister bis zur Anzeige.

### Was diese Prüfung nicht abdeckt

Sie prüft den **Gain bei 0 Hz**. Sie sagt nichts über den Frequenzgang — dafür
ist Abschnitt B da. Und sie prüft nicht die Signalkette dahinter (Bandfilter,
FFT, Detektoren); die ist bereits durch die 151 Tests in
`firmware/test/test_dsp/` abgedeckt und braucht keine Hardware.

## B1. Erste Messungen im Einmachglas — **Scheitelfaktor bestätigt**

Lautsprecher auf ein Weckglas gelegt, Sensor darin. Das ist ein grobes
Pistonphon: der Sensor sieht den Druck in einem geschlossenen Volumen, also
deutlich mehr als den Freifeldpegel.

| Ton | gemessen | Pegel | max | peak | **peak − max** |
|---|---|---|---|---|---|
| 80 Hz | 79,82 Hz | 125,7 dB | 125,8 | 128,9 | **+3,1 dB** |
| 60 Hz | 60,93 Hz | 119,6 dB | 127,4 | 130,4 | **+3,0 dB** |
| 50 Hz | 49,90 Hz | 117,8 dB | 127,3 | 130,4 | **+3,1 dB** |

**Der Scheitelfaktor ist das Ergebnis.** Für einen reinen Sinus beträgt der
Abstand zwischen Spitzen- und Effektivwert theoretisch exakt 3,01 dB.
Gemessen wurden an allen drei Frequenzen 3,0 bis 3,1 dB — an echter Hardware,
mit echtem Schall, unabhängig von jeder Simulation.

Damit sind in einem Zug bestätigt: RMS-Detektor, True-Peak-Detektor,
Bandfilter und Sinc-Korrektur. Es ist dieselbe Eigenschaft, die der
synthetische Test in `firmware/test/` prüft — jetzt am Gerät.

**Was dieser Aufbau nicht leistet:** eine absolute Kalibrierung. Dafür müsste
das verdrängte Volumen ΔV bekannt sein (siehe Abschnitt B). Die Pegelwerte
sind also plausibel, aber nicht rückführbar.

### Frequenzgenauigkeit

| Generator | gemessen | Abweichung |
|---|---|---|
| 50 Hz | 49,90 Hz | −0,20 % |
| 80 Hz | 79,82 Hz | −0,23 % |
| 145 Hz | 145,43 Hz | +0,30 % |
| **60 Hz** | **60,93 Hz** | **+1,55 %** |

Drei Punkte liegen innerhalb von 0,3 %. Der 60-Hz-Punkt fällt heraus — und
zwar **nicht** wegen der Abtastrate: ein Fehler in fs würde bei allen Punkten
denselben Prozentwert ergeben. Die Abweichung entspricht fast genau einem
ganzen FFT-Bin (0,966 Hz), während die Interpolation nachweislich auf 0,02 Bin
genau ist. Wahrscheinlichste Ursache ist die Quelle: ein Generator, der nicht
exakt 60,00 Hz lieferte, oder ein stärkerer Nebenton im Glas. **Vor einer
Schlussfolgerung mit einer zweiten Quelle nachmessen.**

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

## C. Rauschteppich — **gemessen: 87 dB**

Gerät in einem ruhigen Raum betreiben, SPL im Band 10-100 Hz ablesen.

**Ergebnis am fertigen Gerät: 87 dB** im halbwegs ruhigen Raum. Das sind
0,45 Pa effektiv im Band.

Das liegt unter der Erwartung aus dem Datenblatt und ist damit ein gutes
Zeichen. Rechnung zum Vergleich, weißes Rauschen über die Bandbreite verteilt:

| Rauschen breitbandig (OSR 1×) | Abtastrate | davon im Band 10-100 Hz |
|---|---|---|
| 1,3 Pa | 250 Hz | 1,10 Pa = 94,8 dB |
| 1,3 Pa | 500 Hz | 0,78 Pa = 91,8 dB |
| 0,75 Pa | 250 Hz | 0,64 Pa = 90,1 dB |

Gemessen wurden 0,45 Pa. Ein Teil der 87 dB dürfte sogar echter Schall sein —
Lüftung, Verkehr und Gebäudeschwingungen liegen im Infraschallbereich und sind
in einem Wohnraum nie ganz weg. Der tatsächliche Eigenrauschteppich des Geräts
liegt also eher noch darunter.

**Bedeutung für den Messbereich:** bis zum Nutzsignal einer Bassanlage sind es
33 dB (bei 120 dB) bis 73 dB (bei 160 dB). Für brauchbare Genauigkeit sollte
der Messwert etwa 10 dB über dem Teppich liegen — das Gerät ist damit **ab
rund 95 dB verwendbar**, die Zielwerte liegen weit darüber.

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
