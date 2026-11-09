/* DCF77 receiver enclosure — millimetres.
   Print part="base" and part="lid". Both exports are already oriented.
   PCB: quarter of the 88.9 x 96.5 mm ElectroCookie snappable board.
   Peg fit confirmed by user with IMG_1644; smaller USB opening needs a cable check.
   The antenna is captured loosely at its bare ends when the lid is fitted.
*/

/* [View / export] */
part = "assembly"; // [assembly,exploded,cutaway,base,lid,print_layout,fit_check,collision_check]
show_hardware = true;
$fn = 72;

/* [PCB — USB edge faces front, Y=0] */
pcb_width = 44.45;
pcb_depth = 48.25;
pcb_thickness = 1.6;
// Originally inferred from the 2.54 mm grid; printed peg fit confirmed by user.
hole_spacing_x = 39.37;
hole_spacing_y = 44.45;
mount_hole_diameter = 2.2;
peg_diameter = 1.85;
peg_height = 2.8;
standoff_height = 6.0;
standoff_diameter = 4.6;
pcb_lift_clearance = 0.25;
component_height = 12.0;
front_board_gap = 4.0;
rear_board_gap = 1.5;

/* [USB — revised from IMG_1644, check with the actual cable] */
usb_offset_x = 0;
usb_center_above_pcb = 4.5;
usb_projection = 3.5;
// Raise only the original opening's lower edge by 3 mm.
usb_opening_width = 14;
usb_opening_height = 7;
usb_opening_offset_z = 1.5;
usb_corner_chamfer = 2;

/* [Antenna — user measured] */
antenna_length = 60;
antenna_diameter = 10;
coil_diameter = 12;
antenna_end_clearance = 2;
antenna_radial_clearance = 0.3;
antenna_top_clearance = 0.3;
antenna_component_gap = 2;
antenna_y_fraction = 0.72;
cradle_end_inset = 4;
cradle_thickness = 3;
cradle_side_wall = 2.4;

/* [Enclosure / fit] */
wall = 2.4;
floor_thickness = 2.4;
lid_thickness = 2.4;
corner_radius = 4;
lid_clearance = 0.25;
skirt_thickness = 1.2;
skirt_depth = 8;
snap_projection = 0.50;
snap_width = 7;
snap_slot = 0.8;

/* [Hidden] */
eps = 0.02;
inside_w = max(antenna_length + 2*antenna_end_clearance, pcb_width + 16);
inside_d = front_board_gap + pcb_depth + rear_board_gap;
case_w = inside_w + 2*wall;
case_d = inside_d + 2*wall;
pcb_x = (case_w-pcb_width)/2;
pcb_y = wall+front_board_gap;
pcb_z = floor_thickness+standoff_height;
pcb_top = pcb_z+pcb_thickness;
hole_inset_x = (pcb_width-hole_spacing_x)/2;
hole_inset_y = (pcb_depth-hole_spacing_y)/2;
hole_xs = [pcb_x+hole_inset_x,pcb_x+pcb_width-hole_inset_x];
hole_ys = [pcb_y+hole_inset_y,pcb_y+pcb_depth-hole_inset_y];
antenna_x = (case_w-antenna_length)/2;
antenna_y = pcb_y+pcb_depth*antenna_y_fraction;
antenna_z = pcb_top+component_height+antenna_component_gap+coil_diameter/2;
cradle_r = antenna_diameter/2+antenna_radial_clearance;
cradle_span = 2*(cradle_r+cradle_side_wall);
cradle_xs = [antenna_x+cradle_end_inset,antenna_x+antenna_length-cradle_end_inset];
base_h = antenna_z+max(coil_diameter/2+0.8,antenna_diameter/2+antenna_top_clearance+1);
case_h = base_h+lid_thickness;
usb_x = pcb_x+pcb_width/2+usb_offset_x;
usb_z = pcb_top+usb_center_above_pcb;
usb_opening_z = usb_z+usb_opening_offset_z;
snap_ys = [15,case_d-15];
snap_z = base_h-5.5;
skirt_outer_x = wall+lid_clearance;
retainer_bottom = pcb_top+pcb_lift_clearance;
retainer_socket_d = peg_diameter+0.8;

assert(hole_spacing_x < pcb_width && hole_spacing_y < pcb_depth);
assert(peg_diameter < mount_hole_diameter);
assert(peg_height > pcb_thickness+pcb_lift_clearance);
assert(cradle_xs[0]+cradle_thickness/2 < pcb_x-0.4,
       "Antenna cradles must be outside the PCB edges");
assert(antenna_y+cradle_span/2 < case_d-wall-skirt_thickness-lid_clearance);
assert(usb_opening_z-usb_opening_height/2 > floor_thickness);
assert(usb_corner_chamfer >= 0 && usb_corner_chamfer < min(usb_opening_width/2,usb_opening_height));
assert(snap_projection > lid_clearance && snap_projection < wall/2);
echo(case_size_mm=[case_w,case_d,case_h], pcb_mm=[pcb_width,pcb_depth,pcb_thickness],
     hole_spacing_mm=[hole_spacing_x,hole_spacing_y], antenna_mm=[antenna_length,antenna_diameter],
     usb_opening_mm=[usb_opening_width,usb_opening_height],usb_opening_center_height_mm=usb_opening_z);

module rounded_box(w,d,h,r) {
    linear_extrude(h) hull()
        for(x=[r,w-r],y=[r,d-r]) translate([x,y]) circle(r=r);
}

module hole_positions() {
    for(x=hole_xs,y=hole_ys) translate([x,y,0]) children();
}

module x_cylinder(length,diameter) {
    rotate([0,90,0]) cylinder(h=length,d=diameter);
}

module pegs() {
    hole_positions() {
        translate([0,0,floor_thickness-eps])
            cylinder(h=standoff_height+eps,d=standoff_diameter);
        translate([0,0,pcb_z-eps]) cylinder(h=peg_height-0.6+eps,d=peg_diameter);
        translate([0,0,pcb_z+peg_height-0.6])
            cylinder(h=0.6,d1=peg_diameter,d2=peg_diameter-0.6);
    }
}

module usb_cutout() {
    // Original width, top edge and upper chamfers; roof bridge remains 10 mm.
    b=usb_corner_chamfer;
    translate([usb_x,wall+eps,usb_opening_z]) rotate([90,0,0])
        linear_extrude(wall+2*eps) polygon([
            [-usb_opening_width/2,-usb_opening_height/2],
            [ usb_opening_width/2,-usb_opening_height/2],
            [ usb_opening_width/2, usb_opening_height/2-b],
            [ usb_opening_width/2-b,usb_opening_height/2],
            [-usb_opening_width/2+b,usb_opening_height/2],
            [-usb_opening_width/2,usb_opening_height/2-b]]);
}

module left_snap_grooves() {
    for(y=snap_ys)
        translate([wall-0.65,y-snap_width/2-0.3,snap_z-1.2])
            cube([0.65+eps,snap_width+0.6,2.4]);
}

module cradle(x) {
    difference() {
        translate([x-cradle_thickness/2,antenna_y-cradle_span/2,floor_thickness-eps])
            cube([cradle_thickness,cradle_span,antenna_z-floor_thickness+eps]);
        translate([x-cradle_thickness/2-eps,antenna_y,antenna_z])
            x_cylinder(cradle_thickness+2*eps,2*cradle_r);
    }
    // Root flares improve adhesion/strength, staying clear of the PCB.
    hull() {
        translate([x-cradle_thickness/2,antenna_y-cradle_span/2,floor_thickness-eps])
            cube([cradle_thickness,cradle_span,0.2]);
        translate([x-cradle_thickness/2,antenna_y-cradle_span/2+1.2,floor_thickness+3])
            cube([cradle_thickness,cradle_span-2.4,0.2]);
    }
}

module base() {
    union() {
        difference() {
            rounded_box(case_w,case_d,base_h,corner_radius);
            translate([wall,wall,floor_thickness])
                rounded_box(inside_w,inside_d,base_h+eps,max(0.8,corner_radius-wall));
            usb_cutout();
            left_snap_grooves();
            translate([case_w,0,0]) mirror([1,0,0]) left_snap_grooves();
            // Rear seam access for a fingernail or a thin plastic pick.
            translate([case_w/2-5,case_d-1.0,base_h-0.7]) cube([10,1.1,0.8]);
        }
        pegs();
        for(x=cradle_xs) cradle(x);
    }
}

module left_snap_bumps() {
    for(y=snap_ys)
        translate([0,y+snap_width/2,0]) rotate([90,0,0])
            linear_extrude(snap_width) polygon([
                [skirt_outer_x+eps,snap_z-1],
                [skirt_outer_x-snap_projection,snap_z],
                [skirt_outer_x+eps,snap_z+1]]);
}

module left_tab_slots() {
    for(y=snap_ys,s=[-1,1])
        translate([wall-0.1,y+s*(snap_width/2+0.8)-snap_slot/2,base_h-skirt_depth-eps])
            cube([lid_clearance+skirt_thickness+0.3,snap_slot,skirt_depth-1+eps]);
}

module lid_assembled() {
    union() {
        translate([0,0,base_h]) rounded_box(case_w,case_d,lid_thickness,corner_radius);
        difference() {
            translate([skirt_outer_x,skirt_outer_x,base_h-skirt_depth])
                rounded_box(case_w-2*skirt_outer_x,case_d-2*skirt_outer_x,
                    skirt_depth+eps,1.6);
            translate([skirt_outer_x+skirt_thickness,skirt_outer_x+skirt_thickness,base_h-skirt_depth-eps])
                rounded_box(case_w-2*(skirt_outer_x+skirt_thickness),
                    case_d-2*(skirt_outer_x+skirt_thickness),skirt_depth+3*eps,0.6);
            left_tab_slots();
            translate([case_w,0,0]) mirror([1,0,0]) left_tab_slots();
        }
        left_snap_bumps();
        translate([case_w,0,0]) mirror([1,0,0]) left_snap_bumps();
        // The lid limits PCB lift, so loose straight pegs need no snap heads.
        hole_positions() difference() {
            translate([0,0,retainer_bottom])
                cylinder(h=base_h-retainer_bottom+eps,d=standoff_diameter);
            translate([0,0,retainer_bottom-eps])
                cylinder(h=peg_height+0.6,d=retainer_socket_d);
        }
        // Stops sit only above the bare rod ends, well away from its winding.
        for(x=cradle_xs)
            translate([x-cradle_thickness/2,antenna_y-3,
                       antenna_z+antenna_diameter/2+antenna_top_clearance])
                cube([cradle_thickness,6,
                      base_h-(antenna_z+antenna_diameter/2+antenna_top_clearance)+eps]);
    }
}

module lid_print() {
    translate([0,case_d,case_h]) rotate([180,0,0]) lid_assembled();
}

module pcb_reference() {
    color("RoyalBlue") difference() {
        translate([pcb_x,pcb_y,pcb_z]) cube([pcb_width,pcb_depth,pcb_thickness]);
        hole_positions() translate([0,0,pcb_z-eps]) cylinder(h=pcb_thickness+2*eps,d=mount_hole_diameter);
    }
    // Approximate components for clearance review; not a PCB CAD model.
    color([0.12,0.16,0.19]) translate([usb_x-9,pcb_y,pcb_top+1.5]) cube([18,23,1.6]);
    color("Silver") translate([usb_x-4.6,pcb_y-usb_projection,usb_z-1.6]) cube([9.2,8.5,3.2]);
    color("LimeGreen") translate([pcb_x+8,pcb_y+5,pcb_top]) cylinder(h=component_height,d=5);
    color([0.15,0.15,0.17]) translate([antenna_x,antenna_y,antenna_z])
        x_cylinder(antenna_length,antenna_diameter);
    // Coil near the right end, matching the antenna leads' side of the PCB.
    color([0.78,0.35,0.13]) translate([antenna_x+antenna_length-25,antenna_y,antenna_z])
        x_cylinder(13,coil_diameter);
}

module fit_check() {
    // Same peg coordinates, height and front wall/opening as the full base.
    difference() {
        union() {
            for(x=hole_xs)
                translate([x-standoff_diameter/2,0,0])
                    cube([standoff_diameter,hole_ys[1]+standoff_diameter/2,floor_thickness]);
            for(y=hole_ys)
                translate([hole_xs[0]-standoff_diameter/2,y-standoff_diameter/2,0])
                    cube([hole_spacing_x+standoff_diameter,standoff_diameter,floor_thickness]);
            translate([hole_xs[0]-4,0,0])
                cube([hole_spacing_x+8,wall,usb_opening_z+usb_opening_height/2+2]);
            pegs();
        }
        usb_cutout();
    }
    // Separate quick cradle gauge: check the bare rod, not the winding.
    translate([case_w+5,0,0]) difference() {
        union() {
            cube([10,cradle_span,2.4]);
            translate([3.5,0,2.4-eps]) cube([cradle_thickness,cradle_span,cradle_r+eps]);
        }
        translate([3.5-eps,cradle_span/2,2.4+cradle_r])
            x_cylinder(cradle_thickness+2*eps,2*cradle_r);
    }
}

if(part=="base") base();
else if(part=="lid") lid_print();
else if(part=="fit_check") fit_check();
else if(part=="print_layout") {
    color([0.18,0.36,0.45]) base();
    color([0.65,0.72,0.75]) translate([case_w+10,0,0]) lid_print();
}
else if(part=="assembly" || part=="exploded" || part=="cutaway") {
    color([0.18,0.36,0.45]) difference() {
        base();
        if(part=="cutaway") translate([-1,-1,pcb_top+0.5])
            cube([case_w+2,antenna_y-cradle_span/2+1,case_h+1]);
    }
    if(show_hardware) pcb_reference();
    if(part!="cutaway") color([0.65,0.72,0.75])
        translate([0,0,part=="exploded" ? 24 : 0]) lid_assembled();
}
else if(part=="collision_check") {
    // Expected positive volume is ONLY this 1 mm^3 sentinel.
    // Coincident seating faces may also appear as zero-volume surfaces.
    translate([-10,-10,0]) cube(1);
    intersection() { base(); lid_assembled(); }
    intersection() { union() { base(); lid_assembled(); } pcb_reference(); }
}
else assert(false,"Unknown part selector");
