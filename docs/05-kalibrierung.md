# 5. Kalibrierung und Verifikation

Das Gerät ist durch das Messprinzip bereits absolut kalibriert (der Sensor
liefert Pa). Zu prüfen sind deshalb nur drei Dinge: **Gain**, **Frequenzgang**
und **Rauschteppich**.

## A. Gain-Prüfung mit Wassersäule (statisch, kostet 5 EUR)

Ein U-Rohr-Manometer erzeugt einen exakt bekannten Druck:
`p = ρ · g · h` (Wasser bei 15 °C: ρ = 999 kg/m³).

| Höhe H₂O | Druck | äquivalenter Pegel |
|---|---|---|
| 5 cm | 490 Pa | 147,8 dB |
| 10 cm | 980 Pa | 153,8 dB |
| 20 cm | 1960 Pa | 159,8 dB |
| 50 cm | 4900 Pa | 167,8 dB |

Aufbau: durchsichtiger Schlauch 6 mm, U-förmig, teilweise mit Wasser gefüllt,
ein Schenkel an den Sensorport (temporär abgedichtet), Höhendifferenz mit dem
Lineal messen.

**Prüfkriterium:** Der vom Sensor gemeldete Rohdruck (Firmware-Debugmodus, DC
nicht weggefiltert) muss auf ±1 % mit der Rechnung übereinstimmen. Damit ist der
gesamte Messbereich bis 168 dB verifiziert — mit Hausmitteln, ohne Referenz-
gerät. Das kann kein Mikrofonsetup.

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
