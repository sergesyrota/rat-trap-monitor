thick = 10;
width = 83;
height = 60;
round_radius = 15;
hole_d = 40;

difference() {
    hull() {
        translate([-width/2, 0, 0]) cube([width, 1, thick]);
        translate([-width/2+round_radius, height-round_radius, 0]) cylinder(r=round_radius, h=thick);
        translate([width/2-round_radius, height-round_radius, 0]) cylinder(r=round_radius, h=thick);
    }
    hull() {
        translate([0,hole_d/3]) cylinder(d=hole_d, h=thick);
        cylinder(d=hole_d, h=thick);
    }
    // pins
    //translate([width/2-4.5, 0, 0]) cube([3, height, 3]);
    translate([-width/2+1.5, 0, thick-3]) cube([3, height, 3]);
    #translate([-width/2+7,0,0]) cube([3, 5, 3]);
    #translate([width/2-10,-0.01,0]) cube([3, 5, 3]);
}