hs = .93;
bs = 9/8.0;

width = 4 + 7/8;
height = 5;

sixtnth = 1/16;

t = 1/8;

module socket() {
    difference() {
        cube([bs,bs,1/4], center=true);      
        cube([hs,hs,hs], center=true);
        cube([1,1/4,.2],center=true);
    }   
}


difference() {
    translate([0,0,-1/16]) cube(size=[width, height, t], center=true);
    
    for (y = [-1.5: 1.5 : 1.5]) {
    for (x = [-1.5 : 1.5 : 1.5]) {
        translate(v = [x,y,0]) {
            cube(size=[bs,bs,bs], center=true);
        }
    }}
}

    for (y = [-1.5: 1.5 : 1.5]) {
    for (x = [-1.5 : 1.5 : 1.5]) {
        translate(v = [x,y,0]) 
        socket();

    }}