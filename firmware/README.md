# Firmware

Die Messkette ist in zwei Schichten geteilt:

| Schicht | Dateien | Hardware nötig? | Status |
|---|---|---|---|
| DSP-Kern | `src/dsp.*`, `src/spectrum.*`, `src/meter.*` | nein | **fertig, 142 Tests grün** |
| Zielsystem | `src/main.cpp` | ja | Gerüst, TODOs für Phase 2 und 4 |

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
pio run -e esp32 -t upload
```

Vorher die Pinbelegung in `include/config.h` und die TFT-Flags in
`platformio.ini` gegen die eigene Boardrevision prüfen — von den
CYD-Boards gibt es mehrere Varianten.

## Referenzwerte neu erzeugen

Nach jeder Änderung am Python-Modell:

```bash
python3 tools/test_dsp_model.py     # erst das Modell absichern
python3 tools/export_reference.py   # dann die Sollwerte neu schreiben
make -C firmware test               # dann die Portierung dagegen prüfen
```

Diese Reihenfolge ist wichtig: die Referenzwerte sind nur so gut wie das
Modell, aus dem sie stammen.
