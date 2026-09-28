# Firmware

Die Messkette ist in zwei Schichten geteilt:

| Schicht | Dateien | Hardware nötig? | Status |
|---|---|---|---|
| DSP-Kern | `src/dsp.*`, `src/spectrum.*`, `src/meter.*` | nein | **fertig, 142 Tests grün** |
| Zielsystem | `src/main.cpp` | ja | läuft, auch ohne Sensor — siehe Demobetrieb |

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

Die Sensorerkennung liest nur die Chip-ID (0x50) auf 0x47 bzw. 0x46. Dafür
braucht es keine Herstellerbibliothek; der richtige Treiber kommt in Phase 2.

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
