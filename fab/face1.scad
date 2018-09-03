hs = .93;
bs = 9/8.0;

width = 5;
height = 5;

sixtnth = 1/16;

t = 1/8;

module socket() {
    difference() {
        cube([bs,bs,3/8], center=true);      
        cube([hs,hs,hs], center=true);
        cube([1,1/4,.3],center=true);
    }   
}


difference() {
    cube(size=[width, height, t], center=true);
    
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