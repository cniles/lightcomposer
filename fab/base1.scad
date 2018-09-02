thickness = 1 / 16;

module tri() {
    translate([0,0,-1]) {
    linear_extrude(height=2)
        polygon(points = [[0, 0],[.9, 0],[0, .9]]);
    linear_extrude(height=2)
        polygon(points = [[1, .1],[1, 1],[.1, 1]]);
}    }

// baSe
difference() {
    cube(size=[5,5,thickness]);
    translate([.25,.3,0]) scale([2.7,1.8,1]) tri();
    translate([2.55,3,0]) scale([2.25,1.7,1]) tri();
    translate([2.4,3,0]) scale([2,1.7,1]) mirror([1,0,0]) tri();
    translate([3.25,2.8,0]) scale([1.5,2.5,1]) mirror([0,1,0]) tri();    
    translate([1.8,2.2,0]) scale([1.3,.7,1]) mirror([1,0,0]) tri();    
    translate([1.9,2.2,0]) scale([1.2,.7,1]) tri();    
    
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

wall();

translate([0,5,0]) { rotate([0,0,-90]) { wall(); } }

translate([5,0,0]) { rotate([0,0,90]) { wall(); } }

translate([.25,0,4/16]) {
    translate([0, 1/16,0]) {
        //cube([2+15/16, 2+3/16, .5]);
    }
    //translate([0,5-(2+4/16),0]) { cube([5+7/16, 2+3/16, .5]); }
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
}
