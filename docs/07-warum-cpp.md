# 7. Warum C++ auf dem Gerät und nicht durchgehend Python

Berechtigte Frage — das Python-Modell funktioniert ja. Die Antwort in einem
Satz: **Python bleibt, es wechselt nur die Rolle.** Es ist ab jetzt der
Maßstab, an dem die Gerätesoftware gemessen wird, statt selbst auf dem Gerät zu
laufen.

## Gemessen, nicht behauptet

Dieselbe Messkette, derselbe Server, identische Signale:

| | pro Sample | Echtzeitfaktor |
|---|---|---|
| Python-Modell (CPython) | 8,4 µs | 192× |
| C++-Portierung | 0,14 µs | ~11 800× |

Faktor **50–87** zugunsten von C++ (`make -C firmware bench` gegen den
Benchmark in `tools/`).

Hochgerechnet auf den ESP32 bei 240 MHz — grob geschätzt ein Faktor 50–70
langsamer als ein Kern dieses Servers:

| | Rechenlast bei 622 Hz Abtastrate |
|---|---|
| C++ auf dem ESP32 | **unter 1 %** |
| MicroPython auf dem ESP32 (Schätzung) | **grob 50 %** |

Die MicroPython-Zahl ist eine Abschätzung, keine Messung — MicroPython läuft in
diesem Container nicht. Sie ist aber auch nicht der eigentliche Grund.

## Der eigentliche Grund ist nicht Tempo, sondern Pünktlichkeit

Der FIFO des BMP581 fasst **32 Werte**. Bei 622 Hz sind das **51 Millisekunden**.
Wird er nicht rechtzeitig geleert, gehen Messwerte verloren — und zwar leise.
Das Gerät zeigt dann einen falschen Pegel an, ohne dass man es merkt.

Drei Dinge konkurrieren um diese 51 ms: das Auslesen des Sensors, der
Display-Refresh von 320×240 Pixeln und das SD-Logging.

- In C++ mit FreeRTOS wird der Sensor-Task mit hoher Priorität auf Kern 0
  festgenagelt, die Anzeige läuft auf Kern 1. Der Sensor kommt garantiert dran.
- In MicroPython teilen sich alle denselben Interpreter, und der
  Garbage Collector hält ihn unvorhersehbar für einige Millisekunden an.
  Jede Fließkommazahl wird dort auf dem Heap angelegt — bei 622 Werten pro
  Sekunde durch acht Biquads erzeugt das genau den Allokationsdruck, der den
  GC häufig laufen lässt.

Ein Messgerät, das gelegentlich still Samples verliert, ist kein Messgerät.

## Dazu kommt

- **Treiber:** Die Bosch-API für den BMP581 und `TFT_eSPI` sind C beziehungsweise
  C++. Der schnelle Display-Pfad existiert nur dort.
- **Fließkomma:** Der ESP32 hat eine Recheneinheit für einfache Genauigkeit.
  C++ nutzt sie direkt; MicroPython schiebt Objekte über den Heap.

## Was Python weiterhin macht

Das ist kein Wegwerfen, sondern der übliche Ablauf: Modell in der Sprache
bauen, in der Ausprobieren billig ist — portieren — Portierung gegen das Modell
prüfen.

| Werkzeug | Aufgabe |
|---|---|
| `tools/dsp_model.py` | Referenzmodell, 99 eigene Tests |
| `tools/export_reference.py` | erzeugt `firmware/test/reference_vectors.h` |
| `firmware/test/test_dsp/` | prüft die C++-Seite gegen genau diese Sollwerte |
| `tools/verify_log.py` (Phase 8) | wertet die Messlogs am PC aus |

Ohne diesen Umweg hätte man zwei Implementierungen, denen man beiden glauben
muss. Mit ihm hat man eine Referenz und eine Übersetzung mit hartem
Abnahmekriterium: **142 Tests**, darunter ein goldener Vektor aus 64
Ein-/Ausgangswerten, den die C++-Seite auf 0,007 Pa genau reproduziert.

Das war den Aufwand schon wert: die drei Konstruktionsfehler aus
[06-phase1-ergebnisse.md](06-phase1-ergebnisse.md) und die Nyquist-Regel unten
sind alle im Python-Modell gefunden worden, wo ein Durchlauf Sekunden dauert —
nicht auf dem Board, wo jeder Versuch Flashen und Nachmessen bedeutet.

## Neuer Befund aus der Portierung

Die Nyquist-Prüfung war zu lax. Bei 200 Hz Abtastrate liegt die obere
Bandgrenze von 100 Hz **exakt auf Nyquist**; dort entartet der Tiefpass
(w₀ = π ⇒ α = 0, das Filter verliert seine Wirkung) — und schon knapp darunter
wird die Flanke unbrauchbar verzerrt.

Jetzt gilt: **obere Bandgrenze ≤ 0,4 × Abtastrate**, also mindestens 2,5
Abtastwerte pro Periode der höchsten Messfrequenz. Ein unzulässiges Band wird
abgelehnt, statt still ein kaputtes Filter zu bauen.

Praktische Folge: ein BMP390 mit 200 Hz schafft höchstens 80 Hz obere
Bandgrenze — also weder 10–100 noch 5–150 Hz. Das bestätigt die Sensorwahl
BMP581 aus [01-messprinzip.md](01-messprinzip.md).
