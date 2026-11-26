# Troubleshooting

| Symptom | Likely causes | Checks |
|---|---|---|
| No ready banner | Wrong port/baud, firmware not uploaded | Check device manager, board selection and 115200 baud |
| Sender startup timeout | Serial reset or banner missed | Reconnect, check cable and startup output |
| Driver does not move | ENABLE wiring, VMOT supply, current limit, STEP/DIR | Power down before wiring checks; verify supply and common ground |
| Motor vibrates but does not rotate | Coil pairs, current limit, binding | Identify coil pairs and inspect mechanics with power off |
| Axis homes away from switch | Direction inverted or switch mapping wrong | Stop immediately; power down and correct configuration |
| Homing never completes | Switch polarity, obstruction, switch not reached | Verify NC continuity, switch placement and travel |
| Alarm immediately after boot | NC E-stop loop open or home input active | Check wiring and input state before clearing alarm |
| Move rejected as not homed | Homing incomplete or alarm cleared | Resolve cause and run $H |
| Soft-limit error | Target outside configured workspace | Check SVG dimensions and actual usable stroke |
| Pen does not lift/lower | Servo angle or linkage mismatch | Adjust endpoints and check binding |
| Diagonal line skewed | Axis scale mismatch, backlash, loose coupler | Calibrate axes and inspect mechanics |
| SVG has no paths | Artwork contains shapes/text rather than paths | Convert objects/text to paths and flatten transforms |
| Unexpected SVG scale | ViewBox/dimensions mismatch or unsupported transforms | Normalize document dimensions before conversion |

## Safe recovery
1. Stop the job and use the physical hardware E-stop if motion is unsafe.
2. Remove power before touching motor or driver wiring.
3. Identify and correct the cause.
4. Confirm E-stop and NC switch circuits are healthy.
5. Clear alarm with $X only after inspection.
6. Re-home with $H; position is invalid after an alarm.
7. Restart the job from the beginning unless position and drawing state are independently known.
