// Alle Masse des Projekts an einer Stelle.
//
// ============================ ACHTUNG ============================
// Die mit MESSEN markierten Werte sind NICHT verifiziert. Die im Netz
// auffindbaren Angaben zum ESP32-2432S028R widersprechen sich (78x42 mm
// gegen 102x68 mm, und die dort genannte Displayflaeche passt rechnerisch
// nicht zu 2,8" Diagonale). Sie sind hier als plausible Startwerte
// eingetragen, damit das Modell rechnet -- nicht, weil sie stimmen.
//
// Vor dem ersten grossen Druck: Board mit dem Messschieber vermessen,
// die Werte hier eintragen, dann case/fit_test.scad drucken (ca. 15 min).
// Erst wenn der passt, das Gehaeuse drucken (ca. 6 h).
// =================================================================

// ---- Leiterplatte ESP32-2432S028R ----
pcb_l          = 86.0;  // MESSEN: Laenge
pcb_w          = 50.0;  // MESSEN: Breite
pcb_t          =  1.6;  // MESSEN: Dicke
pcb_top_h      =  5.0;  // MESSEN: hoechstes Bauteil ueber der Platine (Displayrahmen)
pcb_bot_h      =  3.0;  // MESSEN: hoechstes Bauteil unter der Platine

// ---- Sichtfenster des Displays ----
// 2,8" bei 320x240 (4:3) ergibt rechnerisch 56,9 x 42,7 mm aktive Flaeche.
win_l          = 58.0;  // MESSEN: Fensterlaenge (etwas groesser als aktiv)
win_w          = 44.0;  // MESSEN: Fensterbreite
win_off_l      =  0.0;  // MESSEN: Versatz des Fensters zur Platinenmitte, Laengsrichtung
win_off_w      =  0.0;  // MESSEN: Versatz quer

// ---- Ausschnitte in den Seitenwaenden ----
usb_w          = 12.0;  // MESSEN: Breite USB-Buchse
usb_h          =  6.0;  // MESSEN: Hoehe
usb_off        =  0.0;  // MESSEN: Versatz aus der Mitte
sd_w           = 15.0;  // MESSEN: microSD-Schlitz
sd_h           =  3.0;
sd_off         = -15.0; // MESSEN: Versatz des SD-Schlitzes aus der Mitte

// ---- Gehaeuse ----
wall           =  2.4;  // Wandstaerke. 3 Perimeter bei 0,4 mm Duese + Reserve
gap            =  0.4;  // Spiel rund um die Platine
floor_t        =  2.4;
bezel_t        =  2.4;
pcb_rest       =  2.0;  // Breite der Auflageleisten unter der Platine
corner_col     =  6.0;  // Kantenlaenge der Eckpfeiler
insert_d       =  4.2;  // MESSEN: Aussendurchmesser der Gewindeeinsaetze M3
insert_h       =  5.0;  // MESSEN: Laenge
screw_d        =  3.2;  // Durchgangsloch M3
screw_head_d   =  6.0;  // Senkung
press_pad      =  8.0;  // Kantenlaenge der Andrueckpads an den Platinenecken
vent_slot_w    =  2.0;
vent_slot_n    =  5;

// ---- Sensorkammer am Geraet (belueftet!) ----
// Diese Kammer ist BEWUSST offen zur Umgebungsluft. Ein dichtes Volumen
// wuerde als Hochpass wirken und genau die tiefen Frequenzen daempfen, um
// die es hier geht. Nicht verwechseln mit der Kalibrierkammer in
// sensor_chamber.scad -- die ist dicht, weil sie das genau soll.
pod_l          = 28.0;
pod_w          = 26.0;
pod_h          = 12.0;
pod_off_l      = 24.0;  // Versatz aus der Gehaeusemitte, Laengsrichtung
sensor_pcb_l   = 25.4;  // MESSEN: Breakout-Platine, hier SparkFun Qwiic (1 x 1 Zoll)
sensor_pcb_w   = 25.4;  // MESSEN
foam_t         =  2.0;  // Moosgummi zur Entkopplung
cable_hole_d   =  6.0;

// ---- Kabeldurchfuehrung ----
grommet_d      =  7.0;
strain_post_d  =  4.0;

// ---- Kalibrierkammer (separates Teil, DICHT) ----
cal_inner_d    = 40.0;
cal_inner_h    = 30.0;
cal_wall       =  2.4;
barb_od        =  6.0;  // passend zum Silikonschlauch 6 mm innen
barb_id        =  3.5;
barb_len       = 14.0;

// ---- Druckeinstellungen als Kommentar ----
// Material ASA oder PETG, kein PLA (im Fahrzeug bis 80 C).
// 0,2 mm Schicht, 4 Perimeter, 25 % Infill, keine Stuetzen noetig
// ausser am Barb der Kalibrierkammer.

// Ueberlappung fuer Koerper, die zusammengehoeren. Flaechenbuendig
// aneinanderstossende Volumen ergeben in CGAL eine nicht-mannigfaltige
// Geometrie, die kein Slicer sauber verarbeitet.
ovl = 0.5;

$fn = 48;

// ---- Abgeleitete Groessen, nicht von Hand aendern ----
cav_l  = pcb_l + 2*gap;
cav_w  = pcb_w + 2*gap;
col_span_l = cav_l + corner_col;          // Mittenabstand der Eckpfeiler
col_span_w = cav_w + corner_col;
out_l  = col_span_l + corner_col + 2*wall;
out_w  = col_span_w + corner_col + 2*wall;
cav_h  = pcb_bot_h + pcb_t + pcb_top_h;
// Die Pads reichen von der Blendenunterseite bis knapp auf die Platine.
// 0,3 mm Luft, damit das Moosgummi den Rest uebernimmt und die Platine nicht
// verspannt wird.
press_h = pcb_top_h - 0.3;
out_h  = floor_t + cav_h;
