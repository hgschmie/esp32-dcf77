# DCF77 receiver case

A two-piece enclosure with four plain PCB locating pegs, a snap-on lid, a USB-C
cable opening, and two cradles holding the ferrite antenna above the PCB. There
are no screws, separate fasteners, or LED window. The closed case is
**68.8 × 58.55 × 39.2 mm**.

![Interior cutaway — the front walls are removed only in this illustration](output/cutaway.png)

## Files

- `dcf77_case.scad`: editable parametric model; opens in assembled view.
- `output/base.stl` and `output/lid.stl`: print one of each, in millimetres.
- `output/print_layout.stl`: the same two parts arranged together on the bed.
- `output/fit_check.stl`: a low-material PCB/USB fit frame and separate rod-cradle gauge.
- `output/assembly.png`, `exploded.png`, `cutaway.png`, `print_layout.png`, and
  `fit_check.png`: reference views. The illustrated electronics are approximate.
- `output/validation.json`: mesh and nominal interference-check results.
- `export.py`: regenerates the STL files and previews with the installed OpenSCAD.

## Dimensions and confidence

The user confirmed that the four pegs fit the real board in `IMG_1644.jpeg`, and
that the USB opening's width and clearance above the connector are satisfactory.
As requested, only the bottom edge has been raised 3 mm: the opening is now
14 × 7 mm. Its width, top edge, upper chamfers, and the peg geometry are unchanged.
The revised lower edge still needs a check with the actual cable.

| Feature | Model value | Basis |
| --- | --- | --- |
| PCB width × depth | 44.45 × 48.25 mm | User's full-board specification divided in half along both axes |
| PCB thickness | 1.6 mm | User's specification |
| Mounting-hole diameter | 2.2 mm | User's specification |
| Mounting-hole centre spacing | 39.37 × 44.45 mm | Printed fit confirmed by user with IMG_1644; not a manufacturer dimension |
| Peg diameter | 1.85 mm, tapered tip | Printed fit confirmed by user |
| Antenna | Ø10 × 60 mm | User's direct measurement |
| Coil envelope | Ø12 × 13 mm | Conservative photo estimate |
| Clearance below PCB | 6 mm above inside floor | Allows for the protruding leads shown in the side photo; measure the longest lead |
| Tallest component | 12 mm above PCB top | Photo estimate, used to set case height |
| USB horizontal position | Centred on the PCB | Photo estimate |
| USB centre height | 4.5 mm above PCB top | Original photo estimate; reference hardware unchanged |
| USB projection beyond PCB | 3.5 mm | Photo estimate; used for the reference hardware |
| USB opening | 14 × 7 mm, 2 mm upper corner chamfers | Bottom edge raised 3 mm as requested; width and top edge confirmed by user |
| USB opening centre | 1.5 mm above the modelled socket centre | Preserves the original opening's top edge while raising its bottom |

The PCB's short edge with USB-C faces the front (`Y=0`). The board sits 4 mm
behind the inside front wall. With the estimated connector projection, its face
is recessed 2.9 mm from the outside. The opening therefore needs to admit the
actual cable's moulded plug body.

The product reference supplied by the user is [Amazon B07ZYTZ48N](https://www.amazon.com/dp/B07ZYTZ48N?th=1).
ElectroCookie also shows the four-quarter layout on its
[manufacturer product page](https://electrocookie.net/product/electrocookie-pcb-prototype-board-snappable-strip-board-with-power-rai/75/).
The listing does not establish the mounting-hole spacing used here.

## Print and check fit

1. Print the revised `fit_check.stl` at 100% scale to check the smaller USB opening.
   It uses approximately 5 cm³ of plastic before slicer settings. The pegs already
   fit the board; check clearance under the leads and insert the actual USB cable
   through the new opening, confirming that it seats fully.
   Rest a bare end of the antenna in the separate cradle gauge.
2. If needed, edit `hole_spacing_x`, `hole_spacing_y`, `peg_diameter`,
   `usb_offset_x`, `usb_center_above_pcb`, or `usb_opening_width/height` in the SCAD
   file. The confirmed peg settings do not need adjustment for this revision.
3. Print the base with its floor on the bed and the lid with its outer top on the
   bed; the exported files already have these orientations. Suggested starting
   settings: ordinary PETG, 0.4 mm nozzle, 0.2 mm layers, 3–4 walls, 5 top/bottom
   layers, and 20–30% infill. PLA can work but the lid tabs need more care.
   Supports should normally be unnecessary: the USB roof has a 10 mm bridge and
   the small latch recesses bridge shallow distances. Verify these in the slicer.
4. Check the lid fit before installing the electronics. `lid_clearance` is
   0.25 mm per side and latch engagement is 0.25 mm. The lid fit is not yet
   physically verified. Increase clearance or reduce `snap_projection` if needed.

## Assemble

1. Lower the PCB onto the four pegs, with USB-C toward the front opening. The
   shoulder of each peg supports the underside of the PCB.
2. Lay the antenna across the rear portion of the board. Its bare end sections
   rest in the two cradles; orient the coil toward the right, near the antenna's
   PCB connection. Keep the fine wires clear of the lid skirt and corner posts.
   Check that the existing wires reach without tension before closing.
3. Fit the lid with its two short antenna stops above the cradles. The four long
   posts align with the PCB pegs. Their sockets clear the peg tips and their ends
   leave 0.25 mm above the board, preventing it from lifting off the pegs.
4. Press the lid evenly until the side tabs engage. The antenna is loosely
   captured between the cradles and lid stops, with about 0.6 mm total vertical
   allowance and 2 mm axial clearance at either end when centred. No clamp presses
   on the coil. Do not force closure against the rod or a trapped wire.
5. To reopen, use the small notch in the rear seam with a fingernail or thin
   plastic pick and gently work around the edge.

The solid lid hides the LED. The cutaway rendering removes the front upper walls
only to show the interior; the printable base has full-height walls.

## Regenerate

Run `python3 export.py` from a normal terminal. In Codex, OpenSCAD and any script
launching it must run outside the sandbox per the installed OpenSCAD skill.
Alternatively choose a `part` in OpenSCAD, render (F6), and export the STL.

The meshes were checked for closed surfaces, consistent winding, positive volume,
and the expected number of separate parts. A solid-intersection check verifies
that the base, seated lid, and simplified hardware do not overlap at the nominal
dimensions. These checks cannot validate measurements inferred from photos,
the solder joints/wires, printer tolerances, or physical latch performance.
