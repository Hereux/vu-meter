// Passungslehre: ein 25 mm langes Stueck der echten Rueckschale.
//
// Druckt in etwa 15 Minuten und prueft genau die Masse, bei denen ein Fehler
// teuer waere:
//   - Spiel um die Platine (gap)
//   - Sitz der Gewindeeinsaetze (insert_d)
//   - Lage und Groesse des USB-Ausschnitts
//   - Wandstaerke und Auflagehoehe
//
// Erst wenn das Teil passt, lohnt der sechsstuendige Druck des Gehaeuses.
// Keine eigene Geometrie: schneidet direkt aus case.scad, kann also nicht
// auseinanderlaufen.

use <case.scad>
include <params.scad>

slab = 25;

intersection() {
    back_shell();
    translate([-out_l/2 + slab/2, 0, out_h/2])
        cube([slab, out_w + 2, out_h*2 + pod_h*2], center = true);
}
