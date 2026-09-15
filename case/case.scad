// Gehaeuse des Bass-SPL-Meters: Rueckschale (Wanne) und Frontblende.
//
// Rendern:  make -C case
// Einzeln:  openscad -D part=\"back\" -o back.stl case/case.scad
//
// part = "back" | "bezel" | "assembly"

include <params.scad>

part = "assembly";

// ---------------------------------------------------------------- Helfer

// Eckpfeiler mit Loch fuer den Gewindeeinsatz
module corner_posts(h, hole = true) {
    for (x = [-1, 1], y = [-1, 1])
        translate([x * col_span_l/2, y * col_span_w/2, 0])
            difference() {
                translate([0, 0, h/2])
                    cube([corner_col, corner_col, h], center = true);
                if (hole)
                    translate([0, 0, h - insert_h])
                        cylinder(d = insert_d, h = insert_h + 1);
            }
}

// Reihe von Lueftungsschlitzen, laengs orientiert
module vent_slots(len, n, slot_w, depth) {
    pitch = len / (n + 1);
    for (i = [1 : n])
        translate([-len/2 + i * pitch, 0, 0])
            rotate([90, 0, 0])
                translate([0, 0, -depth/2])
                    hull() {
                        translate([0, -3, 0]) cylinder(d = slot_w, h = depth);
                        translate([0,  3, 0]) cylinder(d = slot_w, h = depth);
                    }
}

// ---------------------------------------------------------- Sensorkammer
//
// Sitzt aussen auf dem Wannenboden. Auf allen vier Seiten offen, damit der
// Sensor die Kabinenluft sieht. Genau das ist der Punkt: ein geschlossenes
// Volumen wuerde die tiefen Frequenzen wegdaempfen.
module sensor_pod() {
    difference() {
        // 0,5 mm in den Wannenboden hinein ueberlappen, nicht bloss anstossen
        translate([pod_off_l, 0, (-pod_h + ovl)/2])
            cube([pod_l, pod_w, pod_h + ovl], center = true);

        // Innenraum. Endet an der Unterseite des Wannenbodens, damit der
        // Boden seine volle Staerke behaelt und Sensorkammer und
        // Elektronikraum sauber getrennt bleiben.
        translate([pod_off_l, 0, (-pod_h + wall)/2])
            cube([pod_l - 2*wall, pod_w - 2*wall, pod_h - wall], center = true);

        // Lueftung laengs, beide Seiten
        for (y = [-1, 1])
            translate([pod_off_l, y * pod_w/2, -pod_h/2])
                vent_slots(pod_l - 2*corner_col, vent_slot_n, vent_slot_w, wall*3);

        // Lueftung quer, beide Stirnseiten
        for (x = [-1, 1])
            translate([pod_off_l + x * pod_l/2, 0, -pod_h/2])
                rotate([0, 0, 90])
                    vent_slots(pod_w - 2*corner_col, 3, vent_slot_w, wall*3);
    }
    // Auflageleisten fuer die Sensorplatine, darauf kommt das Moosgummi
    for (x = [-1, 1])
        translate([pod_off_l + x * (sensor_pcb_l/2 - 1), 0, -pod_h + wall])
            cube([2, sensor_pcb_w, wall*2 + ovl], center = true);
}

// ------------------------------------------------------------ Rueckschale

module back_shell() {
    difference() {
        union() {
            // Aussenkoerper
            translate([0, 0, out_h/2])
                cube([out_l, out_w, out_h], center = true);
            sensor_pod();
        }

        // Innenraum: bis an die Waende, nicht nur um die Platine herum.
        // Sonst blieben die Stirnseiten als massive Bloecke stehen und der
        // USB-Ausschnitt waere ein 8 mm tiefer Tunnel, in den kein Stecker
        // passt. Die Eckpfeiler stehen frei im Innenraum und dienen
        // gleichzeitig als seitliche Anschlaege fuer die Platine: ihre
        // Innenflaechen liegen genau auf dem Platinenumriss plus Spiel.
        translate([0, 0, floor_t + cav_h/2 + 1])
            cube([out_l - 2*wall, out_w - 2*wall, cav_h + 2], center = true);

        // Kabeldurchfuehrung Sensor: Wannenboden -> Kammer
        translate([pod_off_l, 0, -1])
            cylinder(d = cable_hole_d, h = floor_t + 2);

        // USB-Buchse, Schmalseite
        translate([-out_l/2, usb_off, floor_t + pcb_bot_h + pcb_t + usb_h/2])
            cube([wall*4, usb_w, usb_h], center = true);

        // microSD, Laengsseite. Kein rotate(): das vertauscht Breite und
        // Wanddurchbruch.
        translate([sd_off, -out_w/2, floor_t + pcb_bot_h + pcb_t/2])
            cube([sd_w, wall*4, sd_h + pcb_t], center = true);

        // Kabeldurchfuehrung Versorgung
        translate([out_l/2, 0, floor_t + cav_h/2])
            rotate([0, 90, 0])
                cylinder(d = grommet_d, h = wall*4, center = true);

        // Lueftung der Elektronikkammer, sonst staut sich die Waerme des
        // Spannungswandlers. Nur auf der Seite gegenueber dem
        // microSD-Ausschnitt, sonst schneiden sich die beiden.
        translate([0, out_w/2, floor_t + cav_h/2])
            vent_slots(out_l - 4*corner_col, vent_slot_n, vent_slot_w, wall*4);
    }

    // Eckpfeiler mit Einsatzloch. Starten ovl unterhalb der Bodenoberseite.
    translate([0, 0, floor_t - ovl]) corner_posts(cav_h + ovl);

    // Auflageleisten fuer die Platine. Laufen um ovl in die Eckpfeiler
    // hinein, damit sie nicht nur flaechenbuendig anstossen.
    for (x = [-1, 1])
        translate([x * (cav_l/2 - pcb_rest/2), 0, floor_t + pcb_bot_h/2])
            cube([pcb_rest, cav_w - 2*corner_col + 2*ovl, pcb_bot_h],
                 center = true);
    for (y = [-1, 1])
        translate([0, y * (cav_w/2 - pcb_rest/2), floor_t + pcb_bot_h/2])
            cube([cav_l - 2*corner_col + 2*ovl, pcb_rest, pcb_bot_h],
                 center = true);

    // Zugentlastung: zwei Pfosten im Innenraum, dazwischen ein Kabelbinder.
    // Direkt hinter der Kabeldurchfuehrung, damit Zug am Kabel nicht auf der
    // Loetstelle landet.
    // Auf col_span_l/2 gesetzt: mittig zwischen Auflageleiste und Aussenwand,
    // zu beiden je 1 mm Luft. Weiter innen lag der Zylinder exakt tangential
    // an der Leistenflaeche -- eine Beruehrung ohne Durchdringung, und genau
    // daran scheitert CGAL.
    for (y = [-1, 1])
        translate([col_span_l/2, y * 6, floor_t - ovl])
            cylinder(d = strain_post_d, h = 6 + ovl);
}

// ------------------------------------------------------------ Frontblende

module bezel() {
    difference() {
        translate([0, 0, bezel_t/2])
            cube([out_l, out_w, bezel_t], center = true);

        // Sichtfenster
        translate([win_off_l, win_off_w, -1])
            cube([win_l, win_w, bezel_t + 2], center = true);

        // Schraubenlöcher mit Senkung
        for (x = [-1, 1], y = [-1, 1])
            translate([x * col_span_l/2, y * col_span_w/2, 0]) {
                translate([0, 0, -1]) cylinder(d = screw_d, h = bezel_t + 2);
                translate([0, 0, bezel_t - 1.6])
                    cylinder(d1 = screw_d, d2 = screw_head_d, h = 1.6 + 0.01);
            }
    }
    // Andruecker: vier Pads an den Platinenecken statt einer umlaufenden
    // Rippe. Eine umlaufende Rippe wuerde auf Bauteile druecken -- wo auf
    // dieser Platine welche sitzen, weiss ich nicht. Die Ecken sind bei
    // praktisch jeder Platine frei, weil dort die Befestigungsloecher liegen.
    // Zwischen Pad und Platine kommt ein Stueck Moosgummi.
    for (x = [-1, 1], y = [-1, 1])
        translate([x * (cav_l/2 - press_pad/2), y * (cav_w/2 - press_pad/2),
                   -press_h/2])
            cube([press_pad, press_pad, press_h], center = true);
}

// ------------------------------------------------------------------ Bauen

if (part == "back")        back_shell();
else if (part == "bezel")  bezel();
else {
    back_shell();
    translate([0, 0, out_h + 12]) bezel();
}
