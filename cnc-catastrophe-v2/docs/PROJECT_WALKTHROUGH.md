# Project walkthrough

Use this as a technical explanation guide. Adapt it to what you personally built and verified.

## Short explanation
CNC Catastrophe is a two-axis CNC pen plotter. It converts vector drawings into machine coordinates, sends commands over USB serial to an Arduino Uno, and uses DRV8825 drivers to control two NEMA 17 stepper motors. A servo raises and lowers the pen.

## End-to-end workflow
1. The SVG converter reads supported vector paths and samples curves into line segments.
2. It maps artwork into the configured workspace and generates absolute millimetre commands.
3. The Python sender opens serial and streams one command at a time, waiting for acknowledgement.
4. The Arduino checks machine state and soft limits, then generates step pulses through AccelStepper.
5. DRV8825 modules drive the steppers; the servo controls pen contact.
6. Normally-closed switches establish the machine reference through homing.

## Engineering calculations
Steps/mm = (200 full steps/rev × 16 microsteps) ÷ 8 mm/rev = **400 steps/mm**.

A 10 mm move therefore requires 10 × 400 = **4,000 step pulses**.

A feed of 60 mm/min corresponds to 1 mm/s along the requested path.

## Design decisions to discuss
- STEP/DIR drivers provide straightforward pulse-based motor control.
- NC switches allow an open wire to be detected as a fault with pull-up logic.
- Stop-and-wait serial avoids overflowing the Uno's small input buffer.
- Curves are sampled into G1 segments to keep the controller protocol compact.
- Soft limits are used because the current pin map has negative-end home switches only.
- A hardware E-stop must operate independently of Arduino firmware.

## Be transparent about limitations
This is a compact prototype, not an industrial CNC controller. It does not implement arcs, relative positioning, look-ahead planning or a full G-code dialect. Motion profiles approximate coordinated XY movement and require physical validation.

## Interview questions to prepare
- Derive steps/mm and explain what changes it.
- Walk through the homing state machine.
- Explain how NC wiring detects an open circuit.
- Explain software limits versus physical endstops.
- Explain stop-and-wait serial acknowledgements.
- Explain how SVG curves become G1 segments.
- Explain the response to a limit or E-stop event.
- Describe how to measure scale error, backlash, squareness and homing repeatability.
