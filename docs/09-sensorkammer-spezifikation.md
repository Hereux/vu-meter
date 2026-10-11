# Sensorkammer: akustische Spezifikation

Zweck dieses Dokuments: alles festhalten, was die Kammer um den BMP581
leisten muss, **ohne Geometrie vorzugeben**. Das Gehäuse entsteht in Fusion;
hier steht nur, woran man dort nicht denken würde und was sich nicht
nachschlagen lässt.

Nachrechnen mit `python3 tools/chamber_calc.py --volume … --area … --wall …`
(Volumen in cm³, Fläche in mm², Wand in mm).

---

## 0. Eine Korrektur vorweg

In `case/params.scad` stand als Begründung für die Belüftung, ein dichtes
Volumen wirke als **Hochpass** und dämpfe die tiefen Frequenzen. **Das ist
falsch.** Die Schlussfolgerung stimmt, die Begründung nicht, und mit der
falschen Begründung trifft man beim Umkonstruieren die falschen
Entscheidungen. Deshalb ausführlich:

Ein **Mikrofon** ist ein Differenzsensor. Die Membran trennt außen von einem
Rückvolumen, gemessen wird die Differenz. Ein Entlüftungsloch im Rückvolumen
gleicht bei DC aus, die Differenz wird null → **Hochpass**. Daher die
verbreitete Regel „Entlüftung = Hochpass".

Der **BMP581 ist ein Absolutsensor**. Er misst den Druck dort, wo er liegt,
gegen sein eigenes internes Vakuum. Es gibt kein Rückvolumen, gegen das sich
etwas ausgleichen könnte. Damit dreht sich alles um:

| | dicht | belüftet |
|---|---|---|
| **Mikrofon** (Differenz) | volles Signal | Hochpass |
| **BMP581** (absolut) | **gar kein Signal** | **Tiefpass** |

Eine dichte, starre Kammer liefert dem Absolutsensor überhaupt kein
Wechselsignal — sie hält einfach den eingeschlossenen Druck. Die belüftete
Kammer ist ein **Tiefpass zweiter Ordnung** (Helmholtz-Resonator): bei DC
folgt sie dem Außendruck perfekt, oberhalb der Resonanz entkoppelt sie.

**Daraus folgt die eigentliche Anforderung: die Helmholtz-Frequenz muss
deutlich über dem Band liegen, nicht darunter.** Große Öffnung, kleines
Volumen. Und: Der Fehler im Band ist eine leichte **Überhöhung**, keine
Dämpfung — unterhalb einer Resonanz steigt die Übertragung an.

---

## 1. Harte Anforderungen

| Nr. | Anforderung | Wert | Begründung |
|---|---|---|---|
| A1 | Kammer belüftet, nie dicht | — | Abschnitt 0 |
| A2 | Helmholtz-Frequenz | **≥ 500 Hz**, Ziel ≥ 700 Hz | ≤ 1 dB Fehler bei 150 Hz |
| A3 | Gesamt-Öffnungsfläche | **≥ 80 mm²** bei ≤ 120 cm³ Volumen | folgt aus A2 |
| A4 | Öffnungen aufgeteilt | **≥ 6 statt 1 große** | kleinere Mündungskorrektur |
| A5 | Längste Innenkante | **< 200 mm** | λ/10 bei 150 Hz = 229 mm |
| A6 | Sensor nicht in Sichtlinie einer Öffnung | — | Luftstrahl ≠ Schalldruck |
| A7 | Sensorplatine schwingungsentkoppelt | Moosgummi ≥ 2 mm | MEMS ist beschleunigungsempfindlich |
| A8 | Thermische Trennung von ESP32/Display | Trennwand + eigene Belüftung | Drift der Absolutanzeige |

### Zu A2/A3: das Volumen ist alles, was zusammenhängt

Entscheidend ist **nicht** das Volumen der Sensorkammer allein, sondern jedes
Volumen, das über eine Öffnung damit verbunden ist. Ein 6-mm-Kabelloch in der
Trennwand koppelt die Elektronikkammer voll dazu — aus 29 cm³ werden dann
110 cm³, und die nötige Öffnungsfläche vervierfacht sich.

Zwei saubere Lösungen, beide in Ordnung:

- **Trennen:** Kabeldurchlass mit Silikon oder Heißkleber dicht, beide
  Kammern getrennt belüften. Kleineres Volumen, weniger Öffnungen nötig.
- **Zusammenfassen:** Durchlass offen lassen, aber das Gesamtvolumen in den
  Rechner geben und entsprechend großzügig belüften.

Was **nicht** geht: Durchlass offen und nur die Sensorkammer dimensionieren.

### Zu A3: wie wenig das eigentlich ist

Das ist der beruhigende Teil. Für 110 cm³ und 3 mm Wand:

```
f_H ≥ 500 Hz   →  S ≥  35 mm²   (+0,82 dB bei 150 Hz)
f_H ≥ 700 Hz   →  S ≥  76 mm²   (+0,41 dB)
f_H ≥ 1000 Hz  →  S ≥ 179 mm²   (+0,20 dB)
```

80 mm² sind acht 3,6-mm-Löcher. **Belüftung ist billig** — das ist keine
Stelle, an der man optimieren muss. Oberhalb etwa 1 kHz lohnt es nicht mehr:
größere Löcher bringen mehr Mündungskorrektur mit, die Fläche wächst dann
schneller als der Gewinn. Wenn dir 1 kHz nicht reicht, verkleinere das
Volumen statt die Löcher zu vergrößern.

### Was ausdrücklich *egal* ist

Bei 150 Hz ist λ = 2,3 m, die Kammer also **akustisch kompakt**. Damit gilt:

- **Die Form ist egal.** Nur Volumen und Öffnungsfläche zählen. Rund, eckig,
  L-förmig um die Platine herum — alles gleichwertig. Du bist in Fusion also
  geometrisch völlig frei.
- **Stehende Wellen sind kein Thema** (erste Mode bei knapp 100 mm Länge
  liegt um 1,7 kHz).
- **Die Zähigkeit der Luft ist kein Thema.** Der viskose Pol liegt bei allem,
  was ein FDM-Drucker herstellen kann, bei > 10⁵ Hz. Nur relevant, wenn du
  die Öffnungen mit etwas Dichtem abdecken würdest — siehe A6.
- **Gehäusesteifigkeit ist akustisch kein Thema.** 155 dB sind 1591 Pa Spitze,
  auf eine Wand von 100 × 90 mm also 14 N. Das biegt 3-mm-Kunststoff
  messbar — aber bei guter Belüftung strömt die Luft schneller nach, als die
  Wand sich bewegt, die Volumenänderung wird einfach ausgeglichen. Steifigkeit
  bleibt eine mechanische Frage (Klappern, Dauerfestigkeit), keine
  messtechnische. **Eine dichte Kammer hätte hier ein echtes Problem** — ein
  weiterer Grund für A1.

---

## 2. Der eigentliche Gegner: Luftströmung, nicht Akustik

Die Zahlen oben zeigen: **die Akustik ist der einfache Teil.** Jede halbwegs
großzügige Belüftung erfüllt A2. Die Kammer existiert aus einem anderen Grund.

Ein Sensor ohne Kammer sieht im Auto nicht nur Schalldruck, sondern auch:

- **Luftstrahlen** aus Lüftungsdüsen — direkt im Band, oft stärker als das
  Messsignal
- **Türschläge und Fensterbewegungen** — Drucksprünge von 50 bis 200 Pa,
  entspricht 128 bis 140 dB
- **Fahrtwind** durch Undichtigkeiten
- **Handbewegungen** am Gerät

Alles davon sieht für einen Absolutdrucksensor **exakt wie Schall aus**. Die
Kammer muss diese Strömungen abfangen, ohne den Schalldruck zu behindern. Das
ist der Grund für A6, und dafür gibt es zwei Maßnahmen:

**Sichtlinie brechen.** Keine Öffnung direkt auf den Sensor zeigend. Ein
Versatz oder eine kleine Prallwand reicht; der Schall kommt um die Ecke
(λ = 2,3 m), ein Luftstrahl nicht.

**Offenporiger Schaumstoff vor den Öffnungen.** Das ist derselbe Windschutz
wie beim Mikrofon. Akustisch praktisch unsichtbar, solange er offenporig ist,
aber er zerlegt jeden Strahl. 3 bis 5 mm genügen.

> **Wichtig:** offenporig, nicht geschlossenzellig. Moosgummi als Dichtung ist
> geschlossenzellig und macht die Kammer dicht — genau das, was A1 verbietet.
> Für die Öffnungen also Akustikschaum, Filtermatte oder ein Stück
> Mikrofon-Windschutz; Moosgummi nur unter der Platine (A7).

---

## 3. Mechanik und Thermik

**A7, Schwingungsentkopplung.** Bei 150 dB vibriert im Showcar alles. Ein
MEMS-Drucksensor hat eine Querempfindlichkeit gegen Beschleunigung — die
Membran ist eine Masse an einer Feder. Starr auf eine Gehäusewand geschraubt,
misst der Sensor die Gehäusevibration mit und man kann hinterher nicht mehr
trennen, was Schall war und was Rappeln. Sensorplatine deshalb auf Moosgummi,
nicht direkt verschrauben, Kabel mit Schlaufe statt gespannt.

**A8, Thermik.** ESP32 und Displaybeleuchtung setzen etwa 0,5 bis 1 W um. Der
BMP581 ist werkseitig temperaturkompensiert, aber nicht gegen einen
Gradienten über dem Gehäuse. Für die **SPL-Messung ist das egal** — der
DC-Blocker in der Firmware entfernt jede langsame Drift. Für die
**Absolutanzeige** (Luftdruck, Höhe) ist es nicht egal: ein warmer Sensor
zeigt eine Höhe an, die langsam wandert. Trennwand und getrennte Belüftung
beider Kammern lösen das ausreichend.

---

## 4. Checkliste für den Fusion-Entwurf

- [ ] Volumen der Sensorkammer ermittelt (Fusion: *Eigenschaften* am Körper)
- [ ] Entschieden: Kabeldurchlass dicht oder Gesamtvolumen rechnen
- [ ] `chamber_calc.py` mit den echten Werten → f_H ≥ 500 Hz
- [ ] Öffnungen auf ≥ 6 verteilt, keine davon auf den Sensor zeigend
- [ ] Platz für 3 bis 5 mm offenporigen Schaum hinter den Öffnungen
- [ ] Auflage für die Sensorplatine mit 2 mm Moosgummi, nicht starr
- [ ] Kabel mit Zugentlastung und Schlaufe
- [ ] Elektronikkammer getrennt belüftet (Wärme)

---

## 5. Nachweis (Phase 6.5)

Die Spezifikation ist eine Rechnung. Gemessen ist sie erst, wenn der
Frequenzgang vorliegt. Die Rechnung sagt eine Abweichung < 1 dB voraus — das
ist die Behauptung, die zu prüfen ist.

**Verfahren:** Subwoofer mit Sinus durchstimmen, 10 bis 150 Hz in
Terzschritten. Zwei Durchläufe am selben Ort, selber Pegel:

1. Sensor frei, nur an seinem Kabel hängend
2. Sensor im fertigen Gehäuse

Die **Differenz** der beiden Kurven ist der Einfluss der Kammer. Dass der
Subwoofer selbst keinen glatten Frequenzgang hat, fällt dabei heraus — genau
deshalb die Differenzmessung statt einer Absolutmessung.

**Bestanden:** Differenz < 1 dB über das ganze Band und **monoton steigend**
zum oberen Ende hin.

Das Vorzeichen ist der eigentliche Test. Ein Helmholtz-Tiefpass **überhöht**
unterhalb seiner Resonanz. Findest du stattdessen eine **Absenkung** zu hohen
Frequenzen, ist das Modell nicht schuld, sondern eine Annahme falsch: meist
eine Öffnung, die kleiner ist als gezeichnet (Elefantenfuß in der ersten
Schicht), oder ein Schaumstoff, der doch geschlossenzellig ist.

Zeigt sich ein **schmaler Peak** irgendwo im Band, ist es keine Helmholtz-
Resonanz, sondern Mechanik: eine schwingende Wand oder die Sensorplatine auf
ihrem Schaumstoff. Dann A7 nachbessern, nicht die Löcher vergrößern.

---

## 6. Rückfallebene

Falls Phase 6.5 das Kriterium verfehlt und sich die Ursache nicht findet:
Sensor aus dem Gehäuse heraus, in einen kleinen separaten belüfteten Puck an
kurzem Kabel. Volumen dann unter 10 cm³, damit sind 15 mm² Öffnung genug und
die Elektronik ist als Störquelle komplett raus. Kostet den integrierten
Aufbau, löst das Problem aber sicher.
