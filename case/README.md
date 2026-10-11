# Gehäuse und Druckteile

Parametrisches OpenSCAD-Modell. Alle Maße stehen in `params.scad` — das ist
die einzige Datei, die du anfassen musst.

## Maße: gemessen

Die Werte in `params.scad` stammen aus Messungen am echten Board.
**104,8 × 90,0 × 18,2 mm.**

Zwei Punkte sind noch offen:

- **Vorzeichen von `usb_off`.** Aus 19 bzw. 22 mm Randabstand ergibt sich ein
  Versatz von 1,5 mm aus der Mitte. Ob −1,5 oder +1,5 hängt davon ab, welche
  Kante im Modell unten liegt. Die Passungslehre zeigt es.
- **`insert_d` / `insert_h`**, solange keine Gewindeeinsätze da sind — siehe
  unten.

### Drei Werte habe ich interpretiert

| Wert | deine Angabe | eingetragen | warum |
|---|---|---|---|
| `win_off_l` | 8,5 „beidseitig" | **0** | 8,5 ist der *Rand* je Seite, nicht der Versatz. 68 auf 86 mm heißt 9 mm Rand beidseitig — also mittig. |
| `win_w` | 50 „gleichbreit wie Platine" | **48** | Bündig mit der Platinenkante sieht man bei jeder Montagetoleranz den Spalt zur Kavitätswand. 1 mm Überlappung je Seite verdeckt das. |
| `usb_off`, `sd_off` | „berechnen" | **−1,5 / −4,5** | Aus deinen Randabständen ausgerechnet, Rechenweg oben. |

## Verschraubung: mit oder ohne Gewindeeinsätze

`use_inserts` in `params.scad` schaltet um:

- **`false`** (aktuell): schmales Kernloch von 2,5 mm über die volle Pfeilerhöhe.
  Eine selbstschneidende M3 zieht sich ihr Gewinde selbst. Reicht für den
  Anfang, verträgt aber nur wenige Öffnungszyklen.
- **`true`**: kurzes weites Loch zum Einschmelzen der Einsätze. Hält deutlich
  besser und ist bei Vibration im Fahrzeug die richtige Wahl — nachrüsten,
  sobald welche da sind.

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
| `btn_d` | Bohrung des Tasters — bestimmt die Gehäusehöhe |
| `sensor_pcb_l/w` | Breakout-Platine des BMP581 |

## Teile

| Datei | Was | Druckzeit |
|---|---|---|
| `stl/fit_test.stl` | Passungslehre — **das zuerst** | ~15 min |
| `stl/back.stl` | Rückschale mit Sensorkammer oben | ~5 h |
| `stl/bezel.stl` | Frontblende | ~2 h |
| `stl/cal_cup.stl` + `cal_lid.stl` | Kalibrierkammer, siehe unten | ~1,5 h |

## Aufbau

Ein flacher Kasten, **103,6 × 98,2 × 17,0 mm**. Unten die Elektronik mit dem
Display, oben ein durch eine Wand abgetrennter Streifen als Sensorkammer — in
derselben Ebene, nicht als Beule auf der Rückseite. Ein abstehender Pod
verlagert das Gewicht nach hinten, lässt sich nicht ablegen und bricht als
erstes ab.

Die Höhe von 17 mm kommt vom seitlichen Taster: seine 12-mm-Bohrung braucht
Wand drumherum. Der Taster sitzt in der Stirnwand auf Höhe der Sensorkammer —
in der Elektronikkammer wäre kein Platz für seinen Körper.

Stellschrauben für die Größe, falls 98 mm zu viel sind:

- **Kleinerer Sensor.** Die Qwiic-Micro-Variante statt des 1-Zoll-Boards macht
  83,8 statt 98,2 mm (`sensor_pcb_w` in `params.scad`).
- **Flacherer Taster.** Ein 6-mm-Taster mit gedruckter Kappe brächte die Höhe
  zurück auf 12 mm (`btn_d`).

## Die Stromversorgung passt nicht hinein

Über der Platine bleiben nur **5 mm**. Ein DC/DC-Modul ist rund 10 mm hoch,
dazu kämen Sicherungshalter, TVS und Elkos.

Das ist kein Versehen, sondern die bessere Aufteilung: **die Versorgung gehört
in ein eigenes Inline-Gehäuse am Kabel.** Die Sicherung muss ohnehin nah an die
Einspeisung, nicht ans Display. Das Anzeigegerät bleibt so flach und leicht.

## Die zwei Kammern nicht verwechseln

**Sensorkammer am Gerät**: über Schlitze an der Oberkante und im Boden
**offen** zur Umgebungsluft. Genau so soll es sein — ein dichtes Volumen wirkt
als Hochpass und dämpft die tiefen Frequenzen weg, um die es hier geht. Über
die Öffnungen kommt offenporiger Schaumstoff gegen Staub und direkten Luftzug;
der ist bei diesen Frequenzen akustisch transparent.

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
