# 4. Gehäuse (3D-Druck)

## Material

**ASA** oder **PETG** — kein PLA. Im Fahrzeuginnenraum hinter der Scheibe sind
im Sommer 60-80 °C erreichbar, PLA verformt sich ab ~55 °C. ASA ist zusätzlich
UV-stabil.

Druckparameter: 0,2 mm Schicht, 4 Perimeter, 25 % Infill. Das Gehäuse muss
steif sein, sonst schwingt es bei 150 dB selbst mit.

## Aufbau

Zweiteilig, Front + Rückschale, verschraubt mit 8× M3 in eingelassenen
Gewindeeinsätzen (keine direkt gedruckten Gewinde — die reißen bei Vibration
aus).

```
+-----------------------------+
|  [ 2.8" TFT Ausschnitt ]    |   Front: Displayrahmen, 0,5 mm Spalt rundum
|                             |
|   o  Taster                 |
+-----------------------------+
 Rückschale:
   - Sensorkammer mit Druckport (s. u.)
   - Kabeldurchführung mit Zugentlastung
   - Saugnapf-/GoPro-Aufnahme
   - Lüftungsschlitze für den DC/DC-Wandler
```

## Der Druckport — der kritische Teil

Fehler Nummer eins bei solchen Geräten: ein dichtes Gehäuse. Das eingeschlossene
Luftvolumen bildet mit der Leckage einen Hochpass und dämpft genau die
Frequenzen weg, die gemessen werden sollen.

Regeln:

1. Die Sensorkammer ist **bewusst offen** zum Innenraum: mindestens eine
   Öffnung von 6-8 mm Durchmesser, direkt über dem Sensorport.
2. Die Kammer möglichst klein halten (< 2 cm³), Sensor direkt hinter der
   Öffnung.
3. Öffnung mit **offenporigem Schaumstoff** (5 mm, z. B. Mikrofon-Windschutz)
   abdecken: hält Staub und direkten Luftzug ab, ist bei < 200 Hz akustisch
   transparent.
4. Keine Membran, kein Gore-Tex, kein Gewebe mit Strömungswiderstand.
5. Rest der Elektronik in einer getrennten Kammer, damit Lüftungsschlitze dort
   keine Resonanzen in die Sensorkammer bringen.

## Sensorentkopplung

Die Sensorplatine auf 2 mm Moosgummi setzen und nur leicht verschrauben. Bei
150 dB wird das Gehäuse selbst zur schwingenden Masse; ein starr verschraubter
MEMS-Sensor misst diese Beschleunigung als Scheindruck mit.

## Montage im Fahrzeug

- Zwei 30-mm-Saugnäpfe an der Scheibe, oder GoPro-Mount aufs Armaturenbrett.
- **Messposition ist entscheidend und muss dokumentiert werden.** Im Fahrzeug
  entsteht ein Druckkammer-Feld mit stehenden Wellen; zwischen zwei Positionen
  liegen leicht 10 dB. Für reproduzierbare Vergleiche: feste Position
  definieren (üblich in der Szene: Windschutzscheibe mittig, oder
  Kopfstützenhöhe Fahrersitz) und immer dieselbe verwenden.
- Position im Log mit abspeichern (Freitextfeld in der Firmware).

## Dateien

CAD als parametrisches OpenSCAD-Modell unter `case/`, damit Display- und
Sensorvariante ohne Neuzeichnen anpassbar sind. Export als STL nach `case/stl/`.
