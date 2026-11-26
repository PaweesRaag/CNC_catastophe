# Calibration and commissioning

This is a commissioning procedure, not evidence that calibration has already been performed. Record actual measured values only after testing.

## 1. Confirm mechanical travel
Firmware backs off 5 mm after detecting the negative home switch, then defines that released position as X0/Y0. Confirm whether 150 mm means usable travel from this post-homing zero or total physical travel from switch contact to far end. If it is total stroke, the 5 mm release offset leaves approximately 145 mm usable travel. Update firmware limits and converter workspace consistently.

## 2. Electrical checks (power off)
- Confirm motor coil pairs.
- Confirm DRV8825 carrier orientation and VMOT/GND polarity.
- Install the carrier-recommended bulk capacitor with correct polarity and voltage rating.
- Confirm common logic ground.
- Set 1/16 microstepping on both drivers using the exact carrier truth table.
- Set current limit using the carrier's sense-resistor specification and motor phase-current rating. Do not guess VREF.
- Confirm the hardware E-stop independently interrupts drive power.

Never connect or disconnect a stepper motor while its driver is powered.

## 3. Verify NC switch logic
Expected with INPUT_PULLUP: closed/healthy = LOW; actuated or open circuit = HIGH. Verify each switch independently. A broken wire should be detected as a fault.

## 4. Verify axis direction
With the gantry clear, test X and Y individually at low speed. Homing must move toward the negative-end switch. If an axis moves away, power down and correct direction configuration or wiring. Never bypass a switch.

## 5. Calibrate travel
Command a small move, such as 10 mm, and measure actual carriage displacement with a ruler or caliper.

corrected_steps_per_mm = current_steps_per_mm × commanded_distance / measured_distance

Example only: if 10.0 mm is commanded and 9.8 mm measured, multiply the current scale by 10/9.8. Recheck both axes independently.

## 6. Homing repeatability
Run at least five homing cycles per axis and measure the zero-reference spread. Investigate switch mounting, wiring noise, backlash, loose couplers and mechanical flex if results vary.

## 7. Pen calibration
Tune PEN_UP_ANGLE and PEN_DOWN_ANGLE for the actual linkage. Pen-up must clear the paper; pen-down must make a consistent mark without binding or excessive force.

## 8. Results record

| Check | Actual result |
|---|---|
| X steps/mm | Not measured |
| Y steps/mm | Not measured |
| X usable travel | Not measured |
| Y usable travel | Not measured |
| X homing repeatability (5 cycles) | Not measured |
| Y homing repeatability (5 cycles) | Not measured |
| Pen-up clearance | Not measured |
| Pen-down mark | Not measured |
| Hardware E-stop | Not measured |

## 9. Square test
Convert the supplied 50 mm square, inspect the G-code, home, perform a pen-up dry run, then plot at low feed. Measure width, height, squareness and repeatability. Record firmware version and configuration.
