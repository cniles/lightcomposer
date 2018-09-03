thickness = 1 / 16;

module socket_housing() {
    translate([(1+1/8)/2,(1+1/8)/2,(1+1/8)/2]) rotate([0,0,90])
    union() {
        difference() {
            difference() {
                cube([1+1/8,1+1/8,1+1/8],center=true);
                translate([0,0,-1/16]) cube([1,7/16,1+1/8],center=true);
                translate([0,0,-1/32]) cube([.93,.93,2],center=true);
            }
            translate([0,0,-.5*9/8]) for (a = [0:90:90]) {
                translate([])
                rotate([0,90,a])
                scale([2,1,1]) cylinder(h=2,d=.8,$fn=30, center=true);
            }
        }
        difference() {
            translate([0,0, -.5 - 3/32]) cube([1+1/8,1+1/8, thickness], center=true);
            translate([0,0, -.5 - 3/32]) cube([.93, .93, thickness*4], center=true);
        }
    }
}

module rcaport() {
    cube([3/8,3/8,.5]);
    translate([-6/16,3/16,5/16]) rotate([0,90,0]) cylinder(d=5/16, h=3/8, $fn=20);
}

module spacer() { cube(size=3/16); }

module wall() {
    translate(v=[0,0,0]) {
        difference() {
            cube(size=[thickness,5,3+7/8]); 
            translate(v=[-.25,2.5 ,3+7/8]) { scale([1,1,1.6]) { rotate([0,90,0]) {
                cylinder(d=4.5, $fn=50);
            }}}
        }        
        cube([.25,.25,3+7/8]);
    }
}

module tri() {
    translate([0,0,-1]) {
    linear_extrude(height=2)
        polygon(points = [[0, 0],[.9, 0],[0, .9]]);
    linear_extrude(height=2)
        polygon(points = [[1, .1],[1, 1],[.1, 1]]);
}    }

module tri2() {
    rotate([]) linear_extrude(height=1)
        polygon(points = [[0, 0],[1, 0],[0, 1]]);
}

// baSe
/*
difference() {
    cube(size=[5,5,thickness]);
    translate([.25,.3,0]) scale([2.7,1.8,1]) tri();
    translate([2.55,3,0]) scale([2.25,1.7,1]) tri();
    translate([2.4,3,0]) scale([2,1.7,1]) mirror([1,0,0]) tri();
    translate([3.25,2.8,0]) scale([1.5,2.5,1]) mirror([0,1,0]) tri();    
    translate([1.8,2.2,0]) scale([1.3,.7,1]) mirror([1,0,0]) tri();    
    translate([1.9,2.2,0]) scale([1.2,.7,1]) tri();    
    
}

wall();

translate([0,5,0]) { rotate([0,0,-90]) { wall(); } }

translate([5,0,0]) { rotate([0,0,90]) { wall(); } }

translate([.25,0,4/16]) {
    translate([0, 1/16,0]) {
        //cube([2+15/16, 2+3/16, .5]);
    }
    translate([0,5-(2+4/16),0]) { cube([5+7/16, 2+3/16, .5]); }
}

//spacers for 4channel board
translate([.25,1/16,1/16]) { spacer(); }
translate([.25 + 2+15/16 - 3/16,1/16,1/16]) { spacer(); }
translate([.25,1/16 + 2+4/16 - .25,1/16]) { spacer(); }
translate([.25 + 2+15/16 - 3/16,1/16 + 2+4/16 - .25,1/16]) { spacer(); }

//spacers for 8channel board
translate([0,5-(2+5/16),0]) {
translate([.25,1/16,1/16]) { spacer(); }
translate([1.75+.25 + 2+15/16 - 3/16,1/16,1/16]) { spacer(); }
translate([.25,1/16 + 2+4/16 - .25,1/16]) { spacer(); }
translate([1.75+.25 + 2+15/16 - 3/16,1/16 + 2+4/16 - .25,1/16]) { spacer(); }
}*/



// bottom portion

// walls and bottom
difference() {
    union() {
        translate([9,0,0]) scale([4/5,1,1]) rotate([0,0,90]) wall();
        translate([0,5,0]) mirror([0,1,0]) translate([9,0,0]) scale([4/5,1,1]) rotate([0,0,90]) wall();
        translate([9,0,0]) mirror([1,0,0]) wall();
        difference() { 
            translate([5,0,0]) cube([4,5,thickness]);
            translate([5.75,.5,thickness*-.5]) cube([2.75,4,thickness*2]);
            translate([5.5,.5,thickness*-.5]) cube([1,2,thickness*2]);   
        }
    }   
    translate([8.4,1.75,-.5]) cube([1,1/8,1]);
}
// right side fastener
translate([4.75,4.75-1/16,7/8]) {
    difference() {
        cube([.45,.25,3]);
        translate([.5,-.154,-.01]) rotate([0,-90,0])  scale([1,.4,1]) tri2();
        rotate([-90,-90,0]) scale([1,.25,1]) tri2();
    }
}

// pi power socket housing
translate([9-(1.5),7/16,thickness]) {
    socket_housing();
}

// pi board spacers
translate([9-(2+2/16+.25),5-(2+10/16),thickness]) {
    spacer();
    translate([2,0,0]) spacer();
    translate([0,2+6/16,0]) spacer();
    translate([2,2+6/16,0]) spacer();
    translate([0,0,-thickness]) difference() {
        cube([2+3/16, 2+9/16,thickness]);
        translate([3/16,3/16,-thickness*.25]) cube([1+12/16, 2+3/16,thickness*2]);
    }
}

// pi boards
/*translate([9-(2+2/16+.25),5-(2+10/16),thickness + 3/16]) {
    translate([-.25,5/16,1+1/32]) rcaport();
    translate([-.25,15/16,1+1/32]) rcaport();
    translate([-.25,0,1]) cube([1+3/16,2+9/16,1/32]);
    cube([2+3/16, 2+9/16,1]);
}*/

translate([5,0.0]) cube([.25,.25,3+7/8]);

// 8 channel spacer bottom two
translate([0,5-(2+5/16),0]) {
translate([5.5,1/16,1/16]) { spacer(); }
translate([5.5,2+1/16,1/16]) { spacer(); }
}
