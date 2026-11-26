# Serial command protocol

- Baud: 115200, 8-N-1.
- ASCII commands, one per line, terminated by LF or CRLF.
- Firmware prints CNC_CATASTROPHE_READY at startup.
- Accepted commands end with ok.
- Rejected or failed commands begin with error:.
- Sender transmits one command only after the previous command's ok response.

| Command | Meaning |
|---|---|
| $H | Sequentially home X and Y |
| $X | Clear alarm after cause is removed; position remains invalid until homed |
| G0 X10 Y20 | Rapid absolute move in mm |
| G1 X40 Y20 F60 | Linear absolute move at 60 mm/min |
| M3 | Lower pen |
| M5 | Raise pen |
| M114 | Report position, homed and alarm state |
| G21 | No-op; units are always millimetres |
| G90 | No-op; positioning is always absolute |

Semicolon comments are ignored by the Python sender. Only absolute XY moves are supported.
