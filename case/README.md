# Gehäuse und Druckteile

Parametrisches OpenSCAD-Modell. Alle Maße stehen in `params.scad` — das ist
die einzige Datei, die du anfassen musst.

## ⚠️ Zuerst lesen: die Maße sind nicht verifiziert

Die mit `MESSEN` markierten Werte in `params.scad` sind **plausible
Startwerte, keine gemessenen**. Die im Netz auffindbaren Angaben zum
ESP32-2432S028R widersprechen sich (78 × 42 mm gegen 102 × 68 mm), und die dort
genannte Displayfläche passt rechnerisch nicht zu 2,8" Diagonale: bei 320 × 240
im Format 4:3 ergeben sich 56,9 × 42,7 mm aktive Fläche.

Ich konnte das nicht auflösen — die Shop- und Datenblattseiten sind aus meiner
Arbeitsumgebung nicht erreichbar. Also: **nachmessen, bevor du druckst.**

## Ablauf, der Fehldrucke vermeidet

```
1.  Papiervorlage      make drawing    ->  img/template.svg
    Ausdrucken bei 100 % (Skalierung im Druckdialog AUS), Platine drauflegen.
    Kostet ein Blatt Papier.

2.  Maße eintragen     params.scad
    Messschieber an die Platine, Werte ersetzen.

3.  Passungslehre      make fit        ->  stl/fit_test.stl
    ca. 15 min Druckzeit. Prüft Spiel, Gewindeeinsatz, USB-Ausschnitt,
    Wandstärke und Auflagehöhe -- an echter Geometrie, nicht an einer Kopie:
    fit_test.scad schneidet ein 25 mm langes Stück aus der echten Rückschale.

4.  Erst dann          make            ->  alle STLs
    Das Gehäuse braucht etwa 6 Stunden. Die 15 Minuten vorher sind gut angelegt.
```

## Messliste

Mit dem Messschieber an die Platine, Werte in `params.scad` eintragen:

| Parameter | Was messen |
|---|---|
| `pcb_l`, `pcb_w`, `pcb_t` | Platinenumriss und -dicke |
| `pcb_top_h` | höchstes Bauteil **über** der Platine (meist der Displayrahmen) |
| `pcb_bot_h` | höchstes Bauteil **unter** der Platine |
| `win_l`, `win_w` | sichtbare Displayfläche, plus ~1 mm Reserve |
| `win_off_l`, `win_off_w` | Versatz des Displays gegenüber der Platinenmitte |
| `usb_w`, `usb_h`, `usb_off` | USB-Buchse: Breite, Höhe, Versatz aus der Mitte |
| `sd_w`, `sd_h`, `sd_off` | microSD-Schlitz |
| `insert_d`, `insert_h` | die tatsächlich gelieferten Gewindeeinsätze |
| `sensor_pcb_l/w` | Breakout-Platine des BMP581 |

## Teile

| Datei | Was | Druckzeit |
|---|---|---|
| `stl/fit_test.stl` | Passungslehre — **das zuerst** | ~15 min |
| `stl/back.stl` | Rückschale mit Sensorkammer | ~4 h |
| `stl/bezel.stl` | Frontblende | ~2 h |
| `stl/cal_cup.stl` + `cal_lid.stl` | Kalibrierkammer, siehe unten | ~1,5 h |

## Die zwei Kammern nicht verwechseln

**Sensorkammer am Gerät** (`case.scad`, `sensor_pod`): auf allen vier Seiten
**offen**. Genau so soll es sein — ein dichtes Volumen wirkt als Hochpass und
dämpft die tiefen Frequenzen weg, um die es hier geht. Über die Öffnungen kommt
offenporiger Schaumstoff gegen Staub und direkten Luftzug; der ist bei diesen
Frequenzen akustisch transparent.

**Kalibrierkammer** (`sensor_chamber.scad`): **dicht**. Nur in einem
geschlossenen Volumen lässt sich dem Sensor ein bekannter Überdruck vorsetzen.
Details in [../docs/05-kalibrierung.md](../docs/05-kalibrierung.md).

Ein Schraubglas tut es übrigens auch — die gedruckte Kammer hat nur schon die
Schlauchtülle dran.

## Druckeinstellungen

| | |
|---|---|
| Material | **ASA oder PETG. Kein PLA** — im Fahrzeug bis 80 °C, PLA verformt sich ab 55 °C |
| Schichthöhe | 0,2 mm |
| Perimeter | 4 — das Gehäuse muss steif sein, sonst schwingt es bei 150 dB selbst mit |
| Infill | 25 % |
| Stützen | keine, außer an der Schlauchtülle der Kalibrierkammer |
| Orientierung | Rückschale mit dem Boden aufs Bett, Sensorkammer nach oben |

Gewindeeinsätze mit dem Lötkolben bei ~230 °C einschmelzen, senkrecht und ohne
Druck. Direkt gedruckte Gewinde reißen bei Vibration aus.

## Nach dem Zusammenbau

Die Sensorplatine auf 2 mm Moosgummi setzen und nur leicht andrücken. Bei
150 dB wird das Gehäuse selbst zur schwingenden Masse; ein starr verschraubter
Sensor misst diese Beschleunigung als Scheindruck mit.

**Frequenzgang nach dem Einbau erneut messen** (Phase 6.5). Das Gehäuse ist der
wahrscheinlichste Fehlerort im ganzen Projekt.

## Eine Notiz zum Modell selbst

Beim Erstellen sind sechs Fehler aufgefallen, alle erst beim Ansehen der
gerenderten Bilder, nicht beim Kompilieren:

- Der Innenraum war nur um die Platine herum ausgehöhlt — die Stirnseiten
  blieben massiv stehen, der USB-Ausschnitt wäre ein 8 mm tiefer Tunnel
  gewesen, in den kein Stecker passt.
- Ein `rotate()` vertauschte Breite und Wanddurchbruch des microSD-Schlitzes.
- Lüftungsschlitze schnitten in den microSD-Ausschnitt.
- Die Sensorkammer dünnte den Wannenboden auf 1,2 mm aus.
- Die Zugentlastungspfosten standen auf massivem Material außerhalb des
  Innenraums und waren damit wirkungslos.
- Ein Zugentlastungspfosten lag exakt tangential an einer Auflageleiste. Eine
  Berührung ohne Durchdringung — daran scheitert CGAL, und das STL wurde
  nicht-mannigfaltig.

Dass so etwas beim reinen Kompilieren durchgeht, ist der Grund für die
Papiervorlage und die Passungslehre.
