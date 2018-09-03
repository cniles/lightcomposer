t=14/16;
module pi_support() {
    translate([0,t,0]) cube( [3/16, 4/16, 2/16]);
    translate([3/32,3/32,0]) cylinder(d=5/64,h=5/16,$fn=30);
    translate([0,-4/16,0]) {
        cube([3/16, t + 4/16, 3/16]);
        difference() {
            translate([0,- 13/16,3/16]) cube([3/16,1,1/16]);
            translate([3/32,-11/16,0]) cylinder(d=3/32, h=1, $fn=30);
        }
    }
}

pi_support();
