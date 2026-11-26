# Bill of materials and configuration record

This list records the known design components. Exact supplier part numbers, motor current ratings, power-supply rating and mechanical dimensions were not provided; confirm them from the installed hardware rather than guessing.

| Item | Qty | Known configuration | Record / verify |
|---|---:|---|---|
| Arduino Uno | 1 | ATmega328P-compatible Uno board | Board revision and USB interface |
| NEMA 17 stepper motor | 2 | Assumed 200 full steps/rev | Exact model and rated phase current |
| DRV8825 carrier | 2 | 1/16 microstepping configured | Carrier revision and sense-resistor value |
| Lead screw | 2 | 8 mm lead/rev | Actual lead and usable stroke |
| Normally-closed home switch | 2 | One per axis | Switch model, NC terminal and mounting |
| Pen-lift servo | 1 | Signal on D6 | Servo model, supply and mechanical range |
| Motor power supply | 1 | External supply required | Actual voltage/current rating |
| Hardware E-stop | 1 | Latching, independent drive interruption | Contact rating and wiring implementation |
| Bulk capacitors | As required | Per DRV8825 carrier documentation | Capacitance and voltage rating |
| Frame, guides and carriages | As built | Two-axis lead-screw mechanism | Material, dimensions and alignment |
| Couplers, fasteners and wiring | As required | Match installed mechanics/electronics | Exact types and quantities |

## Electrical configuration to record

- Motor supply voltage: not recorded
- Motor rated phase current: not recorded
- DRV8825 sense-resistor value: not recorded
- VREF/current-limit setting: not recorded
- Microstep pin states: 1/16 intended; verify on each carrier
- Logic supply and common-ground arrangement: verify from wiring
- Hardware E-stop circuit: must be independently tested

Do not infer current limits or supply voltage from the motor or driver family name alone.
