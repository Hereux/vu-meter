// 1:1-Papiervorlage zum Gegenpruefen, bevor irgendetwas gedruckt wird.
//
//   make -C case drawing   ->  img/template.svg
//
// Ausdrucken (Skalierung im Druckdialog AUS, 100 %), die Platine drauflegen
// und vergleichen: Umriss, Sichtfenster, Lage der Eckpfeiler. Kostet ein Blatt
// Papier und deckt falsche Masse auf, bevor sie sechs Stunden Druckzeit kosten.

include <params.scad>

// Direkt als 2D-Geometrie aufgebaut. projection() waere falsch -- das
// erwartet 3D-Koerper als Kinder, hier sind es bereits Flaechen.
union() {
    // Umriss des Gehaeuses
    difference() {
        square([out_l, out_w], center = true);
        square([out_l - 2*wall, out_w - 2*wall], center = true);
    }
    // Platinenumriss
    difference() {
        square([pcb_l, pcb_w], center = true);
        square([pcb_l - 0.6, pcb_w - 0.6], center = true);
    }
    // Sichtfenster
    translate([win_off_l, win_off_w, 0])
        difference() {
            square([win_l, win_w], center = true);
            square([win_l - 0.6, win_w - 0.6], center = true);
        }
    // Eckpfeiler
    for (x = [-1, 1], y = [-1, 1])
        translate([x * col_span_l/2, y * col_span_w/2, 0])
            difference() {
                square([corner_col, corner_col], center = true);
                circle(d = insert_d);
            }
    // Sensorkammer
    translate([pod_off_l, 0, 0])
        difference() {
            square([pod_l, pod_w], center = true);
            square([pod_l - 0.6, pod_w - 0.6], center = true);
        }
}
