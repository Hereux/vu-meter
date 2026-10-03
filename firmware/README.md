# Firmware

Die Messkette ist in zwei Schichten geteilt:

| Schicht | Dateien | Hardware nötig? | Status |
|---|---|---|---|
| DSP-Kern | `src/dsp.*`, `src/spectrum.*`, `src/meter.*` | nein | **fertig, 142 Tests grün** |
| Zielsystem | `src/main.cpp` | ja | läuft, auch ohne Sensor — siehe Demobetrieb |
| BMP581-Treiber | `src/bmp581_regs.h` | nein | **Registerwerte und Dekodierung, 19 Tests** |
| BMP581-Bus | `src/bmp581.{h,cpp}` | ja | I²C-Anbindung, nur am Gerät prüfbar |

Der DSP-Kern ist frei von Arduino- und ESP-IDF-Abhängigkeiten. Genau deshalb
läuft derselbe Code im PC-Test, der ihn gegen die Referenzwerte aus dem
Python-Modell prüft.

## Testen ohne Hardware

```bash
make -C firmware test     # 142 Abnahmetests gegen tools/dsp_model.py
make -C firmware bench    # Durchsatzmessung
```

Nur `g++` nötig. Mit PlatformIO alternativ `pio test -e native`.

## Aufs Board bringen

```bash
pio run -e esp32 -t upload --upload-port COM13   # Windows
pio run -e esp32 -t upload --upload-port /dev/ttyUSB0
pio device monitor -b 115200 -p COM13            # Ausgabe mitlesen
```

Wer keine Toolchain einrichten will: der CI-Lauf legt die Binärdateien als
Artefakt `firmware-esp32` ab (Actions-Tab → Lauf öffnen → Artifacts). Flashen
dann mit `esptool`:

```
esptool.py --port COM13 --chip esp32 write_flash \
  0x1000 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

Vorher die TFT-Flags in `platformio.ini` und die Pinbelegung in
`include/config.h` gegen die eigene Boardrevision prüfen — von den CYD-Boards
gibt es mehrere Varianten.

### Hintergrundbeleuchtung

GPIO 21, aktiv HIGH, per PWM geregelt (`cfg::kBacklightDefault`, Vorgabe 80 %).
Der aktuelle Wert steht in der Statuszeile.

**GPIO 21 darf nicht für I²C verwendet werden**, obwohl 21/22 die
Arduino-Standardpins sind: `Wire.begin()` konfiguriert den Pin um und schaltet
die Beleuchtung aus. Das Display zeigt dann ein korrektes, aber dunkles Bild.
Zwei `static_assert` in `include/config.h` verhindern das jetzt zur
Übersetzungszeit — I²C liegt auf 27/22.

## BMP581-Treiber

Aufgeteilt nach Prüfbarkeit:

- **`bmp581_regs.h`** — Registeradressen, Bitpackung der Konfigurationsregister
  und Dekodierung der Rohbytes. Frei von Arduino, deshalb im PC-Test
  abgedeckt. Genau hier verstecken sich Fehler: ein verrutschtes Bit im
  OSR-Register oder ein vertauschtes Byte beim 24-Bit-Wert fällt sonst erst
  am Gerät auf, und dann als unplausible Zahl ohne Hinweis auf die Ursache.
- **`bmp581.{h,cpp}`** — die I²C-Anbindung. Lässt sich nur am Gerät prüfen.

Die Werte stammen aus der offiziellen Bosch-API
([BMP5_SensorAPI](https://github.com/boschsensortec/BMP5_SensorAPI),
`bmp5_defs.h` und `bmp5.c`), nicht aus dem Gedächtnis.

Konfiguration beim Start: Standby → IIR umgangen → Oversampling 1× →
FIFO nur mit Druckwerten → Dauerbetrieb. Die Reihenfolge ist vorgeschrieben,
der Sensor übernimmt Konfigurationswerte sonst nicht zuverlässig.

### Abtastrate wird gemessen, nicht angenommen

Im Dauerbetrieb ignoriert der BMP581 das ODR-Feld und taktet so schnell, wie
das Oversampling zulässt — mit einem internen RC-Oszillator. Die Rate geht
1:1 in die Frequenzanzeige ein, 5 % Fehler bei 40 Hz sind 2 Hz. Beim Start
zählt die Firmware deshalb 30 s lang Messwerte gegen die Uhr des ESP32 und
legt Filter und FFT-Frequenzachse daraus aus.

**Ist die gemessene Rate zu niedrig für das Messband, bricht der Start mit
einer Meldung ab** statt still auf ein engeres Band auszuweichen. Sonst stünde
eine Zahl auf dem Display, die etwas anderes misst als beschriftet. Nötig sind
250 Hz für das Band 10–100 Hz (obere Grenze ≤ 0,4 × Abtastrate).

### FIFO-Überlauf

Der Sensor meldet im Streaming-Betrieb keinen eigenen Überlauf. Ist der FIFO
beim Lesen voll (32 Rahmen), sind mit hoher Wahrscheinlichkeit Messwerte
verloren gegangen — die Statuszeile zeigt dann `OVR` mit Zähler und wird
orange. Still weiterzurechnen wäre falsch: fehlende Werte senken den
angezeigten Pegel.

## Kalibrierschirm

Kurzer Druck auf den Boot-Taster (GPIO 0) wechselt zwischen Messanzeige und
Kalibrierschirm. Dieser zeigt den **Absolutdruck vor dem DC-Blocker** — genau
die Größe, die die Prüfungen aus
[docs/05-kalibrierung.md](../docs/05-kalibrierung.md) brauchen.

| Anzeige | wofür |
|---|---|
| Absolutdruck in hPa und Pa | Wassersäulentest, Plausibilität |
| Differenz zur Referenz, in Pa **und in Metern Höhe** | Stockwerktest |
| Temperatur | Einschwingen nach dem Komprimieren abwarten |
| gemessene Abtastrate | das Ergebnis der Startmessung nachlesen |
| Sensoradresse, Überlaufzähler | Diagnose |

**Langer Druck setzt die Referenz** auf den aktuellen Wert. Danach steht die
Differenz direkt auf dem Display — der Stockwerktest wird damit zum Ablesen
statt zum Rechnen: Referenz setzen, drei Stockwerke hoch, es müssen rund
−120 Pa beziehungsweise +10 m dastehen.

Die Umrechnung nutzt 12 Pa je Meter (Luftdichte am Boden). Das ist eine
Näherung für den Plausibilitätstest, keine Höhenmessung.

Auf der Messanzeige setzt der lange Druck stattdessen die Haltewerte zurück.

## Demobetrieb ohne Sensor

Findet die Firmware beim Start keinen BMP581, schaltet sie automatisch in den
Demobetrieb: ein synthetischer Sinus-Sweep von 20 auf 80 Hz bei 150 dB läuft
durch **dieselbe Messkette** wie später der echte Schall.

Damit lässt sich schon vor dem Sensor prüfen:

- Display, Rotation und Farben stimmen
- die Rechenkette läuft auf echter Hardware in Echtzeit
- die Anzeige zeigt plausible Werte: **150,0 dB** konstant, die Frequenz
  wandert in 12 s von 20 auf 80 Hz und springt dann zurück

Weicht der angezeigte Pegel von 150 dB ab oder hakt die Frequenz, stimmt etwas
in der Kette oder am Timing nicht — das ist der eigentliche Zweck.

Der Demobetrieb startet automatisch, wenn beim Einschalten kein BMP581
antwortet. Die Statuszeile zeigt dann `DEMO`; mit Sensor steht dort
`BMP581 0x47` und die gemessene Abtastrate.

Bleibt das Display dunkel: die serielle Ausgabe alle 5 s zeigt dieselben Werte.
Kommt dort etwas an, läuft die Firmware und nur die Display-Konfiguration
stimmt nicht.

## Referenzwerte neu erzeugen

Nach jeder Änderung am Python-Modell:

```bash
python3 tools/test_dsp_model.py     # erst das Modell absichern
python3 tools/export_reference.py   # dann die Sollwerte neu schreiben
make -C firmware test               # dann die Portierung dagegen prüfen
```

Diese Reihenfolge ist wichtig: die Referenzwerte sind nur so gut wie das
Modell, aus dem sie stammen.
