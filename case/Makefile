# Baut alle STLs und Vorschaubilder aus den OpenSCAD-Quellen.
#
#   make            alle STLs
#   make img        Vorschaubilder
#   make drawing    1:1-Papiervorlage als SVG
#   make fit        nur die Passungslehre (das Teil, das man zuerst druckt)
#   make clean

SCAD    ?= openscad
RENDER  := xvfb-run -a $(SCAD) --imgsize=900,700 --colorscheme=Tomorrow
STL     := stl
IMG     := img

PARTS   := back bezel cal_cup cal_lid fit_test

.PHONY: all img drawing fit clean

all: $(addprefix $(STL)/,$(addsuffix .stl,$(PARTS)))

$(STL)/back.stl: case.scad params.scad | $(STL)
	$(SCAD) -o $@ -D 'part="back"' case.scad
$(STL)/bezel.stl: case.scad params.scad | $(STL)
	$(SCAD) -o $@ -D 'part="bezel"' case.scad
$(STL)/cal_cup.stl: sensor_chamber.scad params.scad | $(STL)
	$(SCAD) -o $@ -D 'part="cup"' sensor_chamber.scad
$(STL)/cal_lid.stl: sensor_chamber.scad params.scad | $(STL)
	$(SCAD) -o $@ -D 'part="lid"' sensor_chamber.scad
$(STL)/fit_test.stl: fit_test.scad case.scad params.scad | $(STL)
	$(SCAD) -o $@ fit_test.scad

fit: $(STL)/fit_test.stl
	@echo "Passungslehre gebaut: $(STL)/fit_test.stl -- die zuerst drucken."

img: | $(IMG)
	$(RENDER) --viewall --autocenter -o $(IMG)/back.png -D 'part="back"' case.scad
	$(RENDER) --camera=0,0,0,125,0,35,300 -o $(IMG)/back_bottom.png -D 'part="back"' case.scad
	$(RENDER) --camera=0,0,0,125,0,35,220 -o $(IMG)/bezel.png -D 'part="bezel"' case.scad
	$(RENDER) --viewall --autocenter -o $(IMG)/assembly.png case.scad
	$(RENDER) --viewall --autocenter -o $(IMG)/fit_test.png fit_test.scad
	$(RENDER) --viewall --autocenter -o $(IMG)/cal_chamber.png sensor_chamber.scad

drawing: | $(IMG)
	$(SCAD) -o $(IMG)/template.svg drawing.scad

$(STL) $(IMG):
	@mkdir -p $@

clean:
	@rm -f $(STL)/*.stl $(IMG)/*.png $(IMG)/*.svg
