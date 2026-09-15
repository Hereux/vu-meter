// Kalibrierkammer fuer die statische Gain-Pruefung aus docs/05-kalibrierung.md.
//
// Diese Kammer ist DICHT -- im Gegensatz zur beluefteten Sensorkammer am
// Geraet. Sie soll dicht sein: nur in einem geschlossenen Volumen laesst sich
// dem Sensor ein bekannter Ueberdruck vorsetzen.
//
// part = "cup" | "lid" | "assembly"
//
// Ein Schraubglas tut es auch. Dieses Teil ist Bequemlichkeit, keine
// Notwendigkeit -- es hat nur schon die passende Schlauchtuelle dran.

include <params.scad>

part = "assembly";

od = cal_inner_d + 2*cal_wall;

// Schlauchtuelle mit zwei Widerhaken
module barb() {
    rotate([0, 90, 0]) {
        difference() {
            union() {
                cylinder(d = barb_od, h = barb_len);
                for (z = [barb_len - 4, barb_len - 9])
                    translate([0, 0, z])
                        cylinder(d1 = barb_od + 1.2, d2 = barb_od - 0.3, h = 3);
            }
            translate([0, 0, -1]) cylinder(d = barb_id, h = barb_len + 2);
        }
    }
}

module cup() {
    difference() {
        union() {
            cylinder(d = od, h = cal_inner_h + cal_wall);
            // Tuelle sitzt oben, damit kein Wasser hineinlaufen kann, falls
            // der Aufbau doch einmal kippt
            translate([od/2 - 1, 0, cal_inner_h - 6]) barb();
        }
        translate([0, 0, cal_wall])
            cylinder(d = cal_inner_d, h = cal_inner_h + 1);
        // Bohrung der Tuelle in den Innenraum
        translate([od/2 - 2, 0, cal_inner_h - 6])
            rotate([0, 90, 0])
                translate([0, 0, -8]) cylinder(d = barb_id, h = 12);
        // Kabelkerbe im Rand, wird mit Knete oder Heisskleber verschlossen
        translate([-od/2, 0, cal_inner_h + cal_wall - 3])
            cube([cal_wall*3, 5, 6], center = true);
    }
}

module lid() {
    difference() {
        union() {
            cylinder(d = od, h = cal_wall);
            // Zentrierlippe, 0,3 mm Spiel
            translate([0, 0, cal_wall])
                cylinder(d = cal_inner_d - 0.6, h = 3);
        }
        // Griffmulde
        translate([0, 0, -1]) cylinder(d = cal_inner_d - 12, h = 1.5);
    }
}

if (part == "cup")        cup();
else if (part == "lid")   lid();
else {
    cup();
    translate([od + 8, 0, 0]) lid();
}
