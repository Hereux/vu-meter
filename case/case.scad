// Gehaeuse des Bass-SPL-Meters: Rueckschale (Wanne) und Frontblende.
//
// Aufbau: ein flacher Kasten. Unten die Elektronik mit dem Display, oben ein
// durch eine Wand abgetrennter Streifen als Sensorkammer -- in derselben
// Ebene, nicht als Beule auf der Rueckseite. Ein abstehender Pod verlagert
// das Gewicht nach hinten, laesst sich nicht ablegen und bricht als erstes ab.
//
// Rendern:  make -C case
// part = "back" | "bezel" | "assembly"

include <params.scad>

part = "assembly";

// ---------------------------------------------------------------- Helfer

// Pfeiler mit Loch fuer den Gewindeeinsatz, an frei waehlbaren Stellen.
module post(x, y, h) {
    // Mit Gewindeeinsatz: kurzes, weites Loch zum Einschmelzen.
    // Ohne: schmales Kernloch ueber fast die ganze Hoehe, in das eine
    // selbstschneidende M3 ihr Gewinde zieht.
    hole_d = use_inserts ? insert_d : screw_pilot_d;
    hole_h = use_inserts ? insert_h + 1 : h - 2;
    translate([x, y, 0])
        difference() {
            translate([0, 0, h/2])
                cube([corner_col, corner_col, h], center = true);
            translate([0, 0, h - hole_h + 1])
                cylinder(d = hole_d, h = hole_h);
        }
}

// Die vier Pfeiler um die Platine herum dienen gleichzeitig als seitliche
// Anschlaege: ihre Innenflaechen liegen auf dem Platinenumriss plus Spiel.
module elec_posts(h) {
    for (sx = [-1, 1], sy = [-1, 1])
        post(sx * col_span_l/2, elec_cy + sy * col_span_w/2, h);
}

// Zwei zusaetzliche Pfeiler am oberen Rand, damit die Blende auch ueber der
// Sensorkammer aufliegt und nicht durchhaengt.
module cham_posts(h) {
    for (sx = [-1, 1])
        post(sx * col_span_l/2, out_w/2 - wall - corner_col/2, h);
}

module all_posts(h) { elec_posts(h); cham_posts(h); }

// Reihe von Lueftungsschlitzen entlang X
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

// ------------------------------------------------------------ Rueckschale

module back_shell() {
    difference() {
        translate([0, 0, out_h/2])
            cube([out_l, out_w, out_h], center = true);

        // Elektronikkammer. Reicht bis zur Oberkante, nicht nur bis
        // Platinenhoehe: out_h richtet sich nach der Tasterbohrung und ist
        // groesser als der Platinenstapel. Sonst bliebe ein Deckel stehen.
        translate([0, elec_cy, floor_t + (out_h - floor_t)/2 + 1])
            cube([out_l - 2*wall, elec_cav_w, out_h - floor_t + 2],
                 center = true);

        // Sensorkammer. Geht bis ganz nach vorn: die Blende schliesst sie,
        // die Luft kommt ueber die Schlitze herein.
        translate([0, cham_cy, floor_t + (out_h - floor_t)/2 + 1])
            cube([out_l - 2*wall, cham_cav_w, out_h - floor_t + 2], center = true);

        // Kabeldurchlass durch die Trennwand
        translate([0, (elec_cy + cham_cy)/2, floor_t + cable_hole_d/2 + 1])
            rotate([90, 0, 0])
                cylinder(d = cable_hole_d, h = out_w, center = true);

        // Lueftung der Sensorkammer: Oberkante und Rueckwand, damit die Luft
        // quer durchziehen kann.
        translate([0, out_w/2, floor_t + (out_h - floor_t)/2])
            vent_slots(out_l - 4*corner_col, vent_slot_n + 2, vent_slot_w, wall*4);
        translate([0, cham_cy, -1])
            linear_extrude(floor_t + 2)
                for (i = [-2 : 2])
                    translate([i * 16, 0])
                        square([vent_slot_w, cham_cav_w - 6], center = true);

        // Seitlicher Taster, auf Hoehe der Sensorkammer
        translate([(btn_at_right ? 1 : -1) * out_l/2, cham_cy, out_h/2])
            rotate([0, 90, 0])
                cylinder(d = btn_d, h = wall*4, center = true);

        // USB-Buchse, Stirnseite
        translate([-out_l/2, elec_cy + usb_off,
                   floor_t + pcb_bot_h + pcb_t + usb_h/2])
            cube([wall*4, usb_w, usb_h], center = true);

        // microSD, untere Laengsseite
        translate([sd_off, -out_w/2, floor_t + pcb_bot_h + pcb_t/2])
            cube([sd_w, wall*4, sd_h + pcb_t], center = true);

        // Kabeldurchfuehrung der Versorgung
        translate([out_l/2, elec_cy, floor_t + cav_h/2])
            rotate([0, 90, 0])
                cylinder(d = grommet_d, h = wall*4, center = true);

        // Lueftung der Elektronikkammer gegen die Waerme des Wandlers
        translate([0, -out_w/2, floor_t + cav_h/2])
            vent_slots(out_l - 4*corner_col, vent_slot_n, vent_slot_w, wall*4);
    }

    translate([0, 0, floor_t - ovl]) all_posts(out_h - floor_t + ovl);

    // Auflageleisten fuer die Platine
    for (sx = [-1, 1])
        translate([sx * (cav_l/2 - pcb_rest/2), elec_cy,
                   floor_t + pcb_bot_h/2])
            cube([pcb_rest, cav_w - 2*corner_col + 2*ovl, pcb_bot_h],
                 center = true);
    for (sy = [-1, 1])
        translate([0, elec_cy + sy * (cav_w/2 - pcb_rest/2),
                   floor_t + pcb_bot_h/2])
            cube([cav_l - 2*corner_col + 2*ovl, pcb_rest, pcb_bot_h],
                 center = true);

    // Auflage fuer die Sensorplatine, darauf kommt das Moosgummi
    for (sx = [-1, 1])
        translate([sx * (sensor_pcb_l/2 - 1), cham_cy, floor_t + wall/2 - ovl])
            cube([2, sensor_pcb_w, wall + ovl], center = true);

    // Zugentlastung hinter der Kabeldurchfuehrung
    for (sy = [-1, 1])
        translate([col_span_l/2, elec_cy + sy * 6, floor_t - ovl])
            cylinder(d = strain_post_d, h = 6 + ovl);
}

// ------------------------------------------------------------ Frontblende

module bezel() {
    difference() {
        translate([0, 0, bezel_t/2])
            cube([out_l, out_w, bezel_t], center = true);

        // Sichtfenster, bezogen auf die Mitte der Elektronikkammer
        translate([win_off_l, elec_cy + win_off_w, -1])
            cube([win_l, win_w, bezel_t + 2], center = true);

        // Schraubenloecher mit Senkung, ueber allen sechs Pfeilern
        for (p = [[-1, elec_cy - col_span_w/2], [1, elec_cy - col_span_w/2],
                  [-1, elec_cy + col_span_w/2], [1, elec_cy + col_span_w/2],
                  [-1, out_w/2 - wall - corner_col/2],
                  [1, out_w/2 - wall - corner_col/2]])
            translate([p[0] * col_span_l/2, p[1], 0]) {
                translate([0, 0, -1]) cylinder(d = screw_d, h = bezel_t + 2);
                translate([0, 0, bezel_t - 1.6])
                    cylinder(d1 = screw_d, d2 = screw_head_d, h = 1.6 + 0.01);
            }
    }

    // Andruecker: vier Pads an den Platinenecken statt einer umlaufenden
    // Rippe. Wo auf dieser Platine Bauteile sitzen, ist unbekannt; die Ecken
    // sind bei praktisch jeder Platine frei.
    for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * (cav_l/2 - press_pad/2),
                   elec_cy + sy * (cav_w/2 - press_pad/2),
                   -press_h/2])
            cube([press_pad, press_pad, press_h], center = true);
}

// ------------------------------------------------------------------ Bauen

if (part == "back")        back_shell();
else if (part == "bezel")  bezel();
else {
    back_shell();
    translate([0, 0, out_h + 14]) bezel();
}
