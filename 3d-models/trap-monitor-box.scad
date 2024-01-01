board_dim = [52.2, 22.0];
board_clearence = 2;
board_thick = 1.8;
parts_height = 16;
wall = 2;
board_support_positions = [
    [0, 0],
    [board_dim[0]-10, 0],
    [8, board_dim[1]-wall],
    [board_dim[0]-wall, board_dim[1]-wall],
    [board_dim[0]*(1/3), board_dim[1]/2],
    [board_dim[0]*(2/3), board_dim[1]/2]
];
screw_body_d=2.2;
screw_length=15;

//bottom_base();
rotate([0,180,0]) translate([0,0,0]) top_cover();


module top_cover() {
    difference() {
        union() {
            hollow_cover();
            // board retention
            hull() { // this one has a few components at the top, so just small
                translate([wall+15, wall, wall/2]) cube([15, wall, 0.1]);
                translate([wall+15, wall, wall*2]) cube([15, 0.1, 0.1]);
            }
            hull() { // also few components to work around
                translate([wall+4, board_dim[1], wall/2]) cube([board_dim[0]-(25.4*0.85), wall, 0.1]);
                translate([wall+4, board_dim[1]+wall, wall*2]) cube([board_dim[0]-(25.4*0.85), 0.1, 0.1]);
            }
        }
        // Battery connector
        translate([board_dim[0]+wall,wall,0]) cube([wall, 8.5, 7]);
        // Trap 6V power connector & catch sensor
        translate([0,wall+7,0]) cube([wall, 15, 7]);
        // ADC connector protrusion
        translate([wall/2,wall+2,0]) cube([wall, 4, 9]);
        /*
        // camera connector
        translate([board_dim[0]+wall,6.5+wall,0]) cube([wall/2, 19, 3]);
        // SD card
        translate([0,13,0]) cube([wall, 13, 2]);
        // flood sensor
        translate([0,45+wall,0]) cube([wall, 10, 8]);
        // on/off switch
        translate([0,49+wall,11]) cube([wall, 14.5, 8.9]);
        // pump connector + depth & pressure sensors
        translate([11+wall,board_dim[1]+wall,0]) cube([37.5, wall, 8]);
        // power brick connector
        translate([board_dim[0]+wall,54+wall,0]) cube([wall, 11, 13]);
        // battery connector
        translate([board_dim[0]+wall,45+wall,0]) cube([wall, 10, 8]);
        // HDMI
        translate([7.5,wall/2,0]) cube([15,wall/2,3]);
        // 2x USB
        translate([39,0,0]) cube([9.5,wall,3]);
        translate([51.5,0,0]) cube([9.5,wall,3]);
        
        // Reset button
        translate([25,65,parts_height-0.02]) cylinder(h=wall*2, d=reset_button_d);
          */      
    }
}

module hollow_cover() {
    difference() {
        union() {
            cube([board_dim[0]+wall*2, board_dim[1]+wall*2, parts_height+wall]);
            // Retention screw
            translate([board_dim[0]/2+wall,wall-screw_body_d/2,0])
                cylinder(h=parts_height+wall, d=screw_body_d*2, $fn=8);
            translate([board_dim[0]/2+wall,wall+board_dim[1]+screw_body_d/2,0])
                cylinder(h=parts_height+wall, d=screw_body_d*2, $fn=8);
        }
        translate([board_dim[0]/2+wall,wall-screw_body_d/2,0]) cylinder(h=screw_length, d=screw_body_d);
        translate([board_dim[0]/2+wall,wall+board_dim[1]+screw_body_d/2,0]) cylinder(h=screw_length, d=screw_body_d);
        // space for the board itself
        translate([wall, wall, -0.01]) cube([board_dim[0], board_dim[1], parts_height]);
        // interlocking lip
        difference() {
            translate([wall*0.4, wall*0.4, -0.01]) cube([board_dim[0]+(wall*0.6*2), board_dim[1]+(wall*0.6*2), wall/2]);
        }
    }
}

module bottom_base() {
    difference() {
        union() {
            hollow_base();            
            // Board support
            for (leg = board_support_positions) {
                translate([wall+leg[0], wall+leg[1], wall]) cube([wall, wall, board_clearence]);
            }
        }
        /*
        // screw holes
        for (leg = screw_positions) {
            translate([wall+leg[0], wall+leg[1], board_clearence+wall-leg_screw_l+0.01]) cylinder(h=leg_screw_l, d=leg_screw_d*1.47);
        }
        // 2x USB
        translate([39,0,board_clearence+board_thick+wall/2]) cube([9.5,wall,3]);
        translate([51.5,0,board_clearence+board_thick+wall/2]) cube([9.5,wall,3]);
        */
    }
    
}

module hollow_base() {
    difference() {
        union() {
            cube([board_dim[0]+wall*2, board_dim[1]+wall*2, board_clearence+board_thick+wall/2]);
            // Interlocking lip
            translate([wall*0.6, wall*0.6, board_clearence+board_thick+wall/2]) cube([board_dim[0]+(wall*0.4*2), board_dim[1]+(wall*0.4*2), wall/2]);
            // Retention screw
            translate([board_dim[0]/2+wall,wall-screw_body_d/2,0]) cylinder(h=board_clearence+wall/2+board_thick, d=screw_body_d*2);
            translate([board_dim[0]/2+wall,wall+board_dim[1]+screw_body_d/2,0]) cylinder(h=board_clearence+wall/2+board_thick, d=screw_body_d*2);
        }
        // space for the board itself
        translate([wall, wall, wall+0.01]) cube([board_dim[0], board_dim[1], board_clearence+board_thick+wall]);
        // Retention screw hole
        translate([board_dim[0]/2+wall,wall-screw_body_d/2,-0.01]) cylinder(h=board_clearence*3, d=screw_body_d+1);
        translate([board_dim[0]/2+wall,wall+board_dim[1]+screw_body_d/2,-0.01]) cylinder(h=board_clearence*3, d=screw_body_d+1);
        //translate([board_dim[0]/2+wall,0,-0.01]) cylinder(h=2, d=screw_head_d*1.2);
    }
}