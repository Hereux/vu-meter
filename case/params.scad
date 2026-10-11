// Alle Masse des Projekts an einer Stelle.
//
// Die Werte stammen aus Messungen am echten Board, nicht mehr aus
// widerspruechlichen Netzangaben. Offen sind nur noch:
//   - Vorzeichen von usb_off (welche Kante im Modell unten liegt)
//   - insert_d / insert_h, solange keine Gewindeeinsaetze da sind
//
// Vor dem grossen Druck trotzdem case/fit_test.scad drucken (ca. 15 min).
// Erst wenn der passt, das Gehaeuse (ca. 5 h).

// ---- Leiterplatte ESP32-2432S028R ----
pcb_l          = 86.0;  // MESSEN: Laenge
pcb_w          = 50.0;  // MESSEN: Breite
pcb_t          =  1.6;  // MESSEN: Dicke
pcb_top_h      = 4.0;   // gemessen: Display, hoechstes Bauteil oben
pcb_bot_h      = 5.0;   // gemessen: Steckkontakt Lautsprecher, mit Toleranz

// ---- Sichtfenster des Displays ----
// 2,8" bei 320x240 (4:3) ergibt rechnerisch 56,9 x 42,7 mm aktive Flaeche.
win_l          = 68.0;      // gemessen
win_w          = 48.0;      // gemessen waren 50 = volle Platinenbreite. Hier 2 mm
                        // schmaler: bei buendigem Fenster sieht man bei jeder
                        // Montagetoleranz den Spalt zur Kavitaetswand.
win_off_l      = 0.0;      // Dein Wert 8,5 war der RAND je Seite, nicht der
                        // Versatz. 68 auf 86 mm ergibt 9 mm Rand beidseitig,
                        // also mittig: Versatz 0.
win_off_w      =  0.0;  // MESSEN: Versatz quer

// ---- Ausschnitte in den Seitenwaenden ----
usb_w          = 9.0;       // gemessen USB-C, Toleranz im Modell ueber gap
usb_h          = 3.0;       // gemessen
usb_off        = -1.5;     // aus 19/22 mm Randabstand: Mitte 23,5 statt 25.
                        // Vorzeichen pruefen -- +1,5, falls die 22er-Kante
                        // im Modell unten liegt (Seite mit dem SD-Schlitz).
sd_w           = 15.0;      // gemessen
sd_h           = 2.0;       // gemessen
sd_off         = -4.5;     // beginnt 31 mm von der USB-Stirnseite, Breite 15,
                        // Mitte also 38,5 von 86 -> 4,5 mm Richtung USB

// ---- Gehaeuse ----
wall           = 3.0;        // Wandstaerke, bewusst stabiler als noetig
gap            =  0.4;  // Spiel rund um die Platine
floor_t        =  2.4;
bezel_t        =  2.4;
pcb_rest       =  2.0;  // Breite der Auflageleisten unter der Platine
corner_col     =  6.0;  // Kantenlaenge der Eckpfeiler
// Verschraubung: mit Gewindeeinsaetzen oder direkt selbstschneidend.
// Einsaetze halten deutlich besser und vertragen mehrfaches Oeffnen;
// direkt ins Material geschraubt reicht fuer den Anfang.
use_inserts    = false;  // auf true, sobald Einsaetze da sind
screw_pilot_d  =  2.5;   // Kernloch fuer selbstschneidende M3 in PETG/ASA

insert_d       = 4.2;       // OFFEN: noch keine Gewindeeinsaetze vorhanden.
                        // Ohne sie halten die Schrauben nicht -- siehe
                        // case/README.md, Abschnitt Verschraubung.
insert_h       =  5.0;  // MESSEN: Laenge
screw_d        =  3.2;  // Durchgangsloch M3
screw_head_d   =  6.0;  // Senkung
press_pad      =  8.0;  // Kantenlaenge der Andrueckpads an den Platinenecken
vent_slot_w    =  2.0;
vent_slot_n    =  5;

// ---- Sensorkammer am Geraet (belueftet!) ----
// Diese Kammer ist BEWUSST offen zur Umgebungsluft. Begruendung steht in
// docs/09-sensorkammer-spezifikation.md und lautet NICHT "Hochpass" --
// das galt frueher hier und war falsch. Der BMP581 ist ein Absolutsensor,
// kein Mikrofon: eine dichte starre Kammer liefert ihm gar kein
// Wechselsignal, eine belueftete wirkt als TIEFpass. Die Helmholtz-
// Frequenz muss darum ueber dem Band liegen -- grosse Oeffnung, kleines
// Volumen. Pruefen mit tools/chamber_calc.py.
// Nicht verwechseln mit der Kalibrierkammer in sensor_chamber.scad --
// die ist dicht, weil sie das genau soll.
// Sie liegt als abgetrennter Streifen OBEN in derselben Ebene wie die
// Elektronik, nicht als Beule auf der Rueckseite. Ein abstehender Pod
// verlagert das Gewicht nach hinten, laesst sich nicht ablegen und bricht
// als erstes ab.
sensor_pcb_l   = 19.0;    // gemessen: DFRobot-Platine
sensor_pcb_w   = 16.0;    // gemessen
                        // wuerde das Gehaeuse um rund 14 mm schrumpfen.
divider_t      =  2.4;  // Trennwand zwischen Kammer und Elektronik
foam_t         =  2.0;  // Moosgummi zur Entkopplung
cable_hole_d   =  6.0;  // Kabeldurchlass durch die Trennwand

// ---- Seitlicher Taster ----
// In der Stirnwand auf Hoehe der Sensorkammer: nach innen steht dort die
// Kammer zur Verfuegung. In der Elektronikkammer waere kein Platz fuer den
// Koerper des Tasters, dort sitzt die Platine.
btn_d          = 12.2;  // MESSEN: Bohrung fuer den Einbautaster
btn_at_right   = true;  // false setzt ihn auf die andere Stirnseite

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
cav_h  = pcb_bot_h + pcb_t + pcb_top_h;

// Elektronikkammer unten, Sensorkammer als Streifen darueber.
elec_cav_w = cav_w + 2*corner_col;        // Innenmass quer
cham_cav_w = sensor_pcb_w + 2*gap + 2;    // Innenmass der Kammer quer
out_w  = wall + elec_cav_w + divider_t + cham_cav_w + wall;

// Mittellinien beider Kammern in Querrichtung
elec_cy = -out_w/2 + wall + elec_cav_w/2;
cham_cy =  out_w/2 - wall - cham_cav_w/2;

// Hoehe muss die Tasterbohrung aufnehmen, sonst bleibt rund um sie keine
// Wand stehen. Muss VOR press_h stehen: OpenSCAD loest Variablen in der
// Reihenfolge der Datei auf und meldet sonst nur eine Warnung, waehrend der
// Wert still undefiniert bleibt.
out_h  = max(floor_t + cav_h, btn_d + 2*wall);

// Die Pads reichen von der Blendenunterseite bis knapp auf die Platine.
// Sie haengen an out_h, nicht an der Platinenhoehe -- sonst erreichen sie die
// Platine nicht mehr, sobald das Gehaeuse wegen des Tasters hoeher wird.
press_h = out_h - (floor_t + pcb_bot_h + pcb_t) - 0.3;
