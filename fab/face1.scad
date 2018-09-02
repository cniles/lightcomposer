hs = 7.0 / 8.0;
bs = 9/8.0;

width = 5;
height = 5;

sixtnth = 1/16;

t = 1/8;

difference() {
    union() {
        cube(size=[width, height, t]);
        for (y = [.5: 1.5 : 4.5]) {
        for (x = [.5 : 1.5 : 4.5]) {
            translate(v = [x-sixtnth,y-sixtnth,0]) {
                cube(size=[bs,bs,.25]);
            }
        }}
    }
    
    for (y = [.5: 1.5 : 5]) {
    for (x = [.5 : 1.5 : 5]) {
        translate(v = [x+sixtnth,y+sixtnth,-1/8]) {
            cube(size=[hs,hs,1]);
        }
    }}
    
}



