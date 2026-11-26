#include <AccelStepper.h>
#include <Servo.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// CNC Catastrophe - Arduino Uno / 2x DRV8825 / NEMA 17
// Absolute XY subset: G0, G1, M3, M5, M114, $H, $X, G21, G90.

// ---------------- Pin map ----------------
constexpr uint8_t X_STEP_PIN = 3;
constexpr uint8_t X_DIR_PIN = 2;
constexpr uint8_t Y_STEP_PIN = 5;
constexpr uint8_t Y_DIR_PIN = 4;
constexpr uint8_t PEN_PIN = 6;
constexpr uint8_t DRIVER_ENABLE_PIN = 8; // DRV8825 ENABLE is active LOW
constexpr uint8_t X_HOME_PIN = 9;
constexpr uint8_t Y_HOME_PIN = 10;
constexpr uint8_t ESTOP_PIN = 11;

// ---------------- Machine configuration ----------------
constexpr float X_STEPS_PER_MM = 400.0f; // 200 full steps * 16 microsteps / 8 mm lead
constexpr float Y_STEPS_PER_MM = 400.0f;
constexpr float X_TRAVEL_MM = 150.0f;
constexpr float Y_TRAVEL_MM = 150.0f;

// NC contacts to GND: healthy/closed = LOW; actuated/open wire = HIGH.
constexpr uint8_t SWITCH_TRIGGERED_LEVEL = HIGH;
constexpr uint8_t ESTOP_TRIGGERED_LEVEL = HIGH;

// Conservative software starting values; validate on the actual machine.
constexpr float MAX_FEED_MM_MIN = 300.0f;
constexpr float DEFAULT_FEED_MM_MIN = 60.0f;
constexpr float PATH_ACCEL_MM_S2 = 10.0f;
constexpr float MAX_AXIS_SPEED_STEPS_S = 2000.0f;
constexpr float HOME_FAST_STEPS_S = 400.0f;
constexpr float HOME_SLOW_STEPS_S = 100.0f;
constexpr float HOME_BACKOFF_MM = 5.0f;
constexpr float HOME_SEARCH_LIMIT_MM = 155.0f;
constexpr float HOME_RELEASE_LIMIT_MM = 10.0f;
constexpr unsigned long HOME_TIMEOUT_MS = 200000UL;
constexpr unsigned long MOVE_TIMEOUT_EXTRA_MS = 15000UL;

constexpr uint8_t PEN_UP_ANGLE = 90;
constexpr uint8_t PEN_DOWN_ANGLE = 0;
constexpr unsigned long PEN_SETTLE_MS = 350UL; // Tune for the installed servo/linkage
constexpr uint8_t SERIAL_BUFFER_SIZE = 96;

AccelStepper xAxis(AccelStepper::DRIVER, X_STEP_PIN, X_DIR_PIN);
AccelStepper yAxis(AccelStepper::DRIVER, Y_STEP_PIN, Y_DIR_PIN);
Servo penServo;

bool machineHomed = false;
bool alarmActive = false;
float modalFeedMmMin = DEFAULT_FEED_MM_MIN;
char serialBuffer[SERIAL_BUFFER_SIZE];
uint8_t serialLength = 0;

bool xHomeTriggered() { return digitalRead(X_HOME_PIN) == SWITCH_TRIGGERED_LEVEL; }
bool yHomeTriggered() { return digitalRead(Y_HOME_PIN) == SWITCH_TRIGGERED_LEVEL; }
bool estopTriggered() { return digitalRead(ESTOP_PIN) == ESTOP_TRIGGERED_LEVEL; }

void driversEnable(bool enable) {
  digitalWrite(DRIVER_ENABLE_PIN, enable ? LOW : HIGH);
}

void raisePen() { penServo.write(PEN_UP_ANGLE); }
void lowerPen() { penServo.write(PEN_DOWN_ANGLE); }

void printError(const __FlashStringHelper *message) {
  Serial.print(F("error: "));
  Serial.println(message);
}

void enterAlarm(const __FlashStringHelper *message) {
  alarmActive = true;
  machineHomed = false;
  raisePen();
  driversEnable(false);
  printError(message);
}

bool checkEmergencyStop() {
  if (estopTriggered()) {
    enterAlarm(F("E-stop circuit open"));
    return false;
  }
  return true;
}

bool waitForSwitch(AccelStepper &axis, uint8_t pin, bool wantTriggered,
                   float speed, long maxTravelSteps) {
  axis.setSpeed(speed);
  axis.setCurrentPosition(0);
  const unsigned long started = millis();
  while ((digitalRead(pin) == SWITCH_TRIGGERED_LEVEL) != wantTriggered) {
    if (!checkEmergencyStop()) return false;
    if (millis() - started > HOME_TIMEOUT_MS) return false;
    if (labs(axis.currentPosition()) > maxTravelSteps) return false;
    axis.runSpeed();
  }
  delay(8);
  return ((digitalRead(pin) == SWITCH_TRIGGERED_LEVEL) == wantTriggered);
}

bool releaseHomeSwitch(AccelStepper &axis, uint8_t pin, long backoffSteps) {
  axis.setMaxSpeed(HOME_FAST_STEPS_S);
  axis.setAcceleration(400.0f);
  axis.moveTo(backoffSteps);
  const unsigned long started = millis();
  while (axis.distanceToGo() != 0) {
    if (!checkEmergencyStop()) return false;
    if (millis() - started > HOME_TIMEOUT_MS) return false;
    axis.run();
  }
  return digitalRead(pin) != SWITCH_TRIGGERED_LEVEL;
}

bool homeOneAxis(AccelStepper &axis, uint8_t pin, long backoffSteps,
                 float stepsPerMm, const __FlashStringHelper *axisName) {
  Serial.print(F("Homing "));
  Serial.println(axisName);
  const long searchLimit = (long)(HOME_SEARCH_LIMIT_MM * stepsPerMm);

  // If already triggered/open, move away only a short, bounded distance.
  // A broken NC wire is also seen as HIGH and must fail safely.
  if (digitalRead(pin) == SWITCH_TRIGGERED_LEVEL) {
    const long releaseLimit = (long)(HOME_RELEASE_LIMIT_MM * stepsPerMm);
    if (!waitForSwitch(axis, pin, false, HOME_FAST_STEPS_S, releaseLimit)) return false;
  }

  // Fast seek toward the negative-end switch.
  if (!waitForSwitch(axis, pin, true, -HOME_FAST_STEPS_S, searchLimit)) return false;
  axis.setCurrentPosition(0);
  if (!releaseHomeSwitch(axis, pin, backoffSteps)) return false;

  // Slow re-approach for repeatability.
  if (!waitForSwitch(axis, pin, true, -HOME_SLOW_STEPS_S, backoffSteps + 1000L)) return false;
  axis.setCurrentPosition(0);
  if (!releaseHomeSwitch(axis, pin, backoffSteps)) return false;

  // Released position is the software machine origin.
  axis.setCurrentPosition(0);
  Serial.print(axisName);
  Serial.println(F(" homed"));
  return true;
}

void homeMachine() {
  if (alarmActive) {
    printError(F("clear alarm with $X after checking the cause"));
    return;
  }
  if (estopTriggered()) {
    enterAlarm(F("E-stop circuit open"));
    return;
  }
  raisePen();
  delay(PEN_SETTLE_MS);
  driversEnable(true);
  machineHomed = false;

  if (!homeOneAxis(xAxis, X_HOME_PIN, (long)(HOME_BACKOFF_MM * X_STEPS_PER_MM),
                   X_STEPS_PER_MM, F("X"))) {
    enterAlarm(F("X homing failed / timeout / switch fault"));
    return;
  }
  if (!homeOneAxis(yAxis, Y_HOME_PIN, (long)(HOME_BACKOFF_MM * Y_STEPS_PER_MM),
                   Y_STEPS_PER_MM, F("Y"))) {
    enterAlarm(F("Y homing failed / timeout / switch fault"));
    return;
  }

  xAxis.setCurrentPosition(0);
  yAxis.setCurrentPosition(0);
  machineHomed = true;
  Serial.println(F("HOMING COMPLETE"));
}

void reportPosition() {
  Serial.print(F("X:")); Serial.print(xAxis.currentPosition() / X_STEPS_PER_MM, 3);
  Serial.print(F(" Y:")); Serial.print(yAxis.currentPosition() / Y_STEPS_PER_MM, 3);
  Serial.print(F(" HOMED:")); Serial.print(machineHomed ? 1 : 0);
  Serial.print(F(" ALARM:")); Serial.println(alarmActive ? 1 : 0);
}

bool moveLine(float targetX, float targetY, float feedMmMin) {
  if (alarmActive || !machineHomed) {
    printError(F("machine must be homed and alarm-free"));
    return false;
  }
  if (targetX < 0.0f || targetX > X_TRAVEL_MM ||
      targetY < 0.0f || targetY > Y_TRAVEL_MM) {
    printError(F("soft travel limit exceeded"));
    return false;
  }
  if (feedMmMin < 10.0f || feedMmMin > MAX_FEED_MM_MIN) {
    printError(F("feed must be between 10 and 300 mm/min"));
    return false;
  }
  if (xHomeTriggered() || yHomeTriggered()) {
    enterAlarm(F("home switch unexpectedly active"));
    return false;
  }
  if (!checkEmergencyStop()) return false;

  const long targetXSteps = lroundf(targetX * X_STEPS_PER_MM);
  const long targetYSteps = lroundf(targetY * Y_STEPS_PER_MM);
  const long dxSteps = targetXSteps - xAxis.currentPosition();
  const long dySteps = targetYSteps - yAxis.currentPosition();
  const float dxMm = dxSteps / X_STEPS_PER_MM;
  const float dyMm = dySteps / Y_STEPS_PER_MM;
  const float pathLength = sqrtf(dxMm * dxMm + dyMm * dyMm);
  if (pathLength < 0.001f) return true;

  // Independent acceleration profiles with proportional limits approximate
  // coordinated line motion. Validate diagonal accuracy before plotting.
  const float pathSpeedMmS = feedMmMin / 60.0f;
  const float xRatio = fabsf(dxMm) / pathLength;
  const float yRatio = fabsf(dyMm) / pathLength;
  const float xMaxSpeed = max(1.0f, min(MAX_AXIS_SPEED_STEPS_S,
      pathSpeedMmS * X_STEPS_PER_MM * xRatio));
  const float yMaxSpeed = max(1.0f, min(MAX_AXIS_SPEED_STEPS_S,
      pathSpeedMmS * Y_STEPS_PER_MM * yRatio));
  const float xAccel = max(1.0f, PATH_ACCEL_MM_S2 * X_STEPS_PER_MM * xRatio);
  const float yAccel = max(1.0f, PATH_ACCEL_MM_S2 * Y_STEPS_PER_MM * yRatio);

  xAxis.setMaxSpeed(xMaxSpeed);
  yAxis.setMaxSpeed(yMaxSpeed);
  xAxis.setAcceleration(xAccel);
  yAxis.setAcceleration(yAccel);
  xAxis.moveTo(targetXSteps);
  yAxis.moveTo(targetYSteps);

  const unsigned long expectedMs =
      (unsigned long)((pathLength / pathSpeedMmS) * 3000.0f) + MOVE_TIMEOUT_EXTRA_MS;
  const unsigned long started = millis();

  while (xAxis.distanceToGo() != 0 || yAxis.distanceToGo() != 0) {
    if (!checkEmergencyStop()) return false;
    if (xHomeTriggered() || yHomeTriggered()) {
      enterAlarm(F("home switch activated during travel"));
      return false;
    }
    if (millis() - started > expectedMs) {
      enterAlarm(F("motion timeout"));
      return false;
    }
    xAxis.run();
    yAxis.run();
  }
  return true;
}

bool parseFloatStrict(const char *text, float &value) {
  if (!text || !*text) return false;
  char *end = nullptr;
  value = strtod(text, &end);
  return end != text && *end == '\0' && isfinite(value);
}

void processCommand(char *line) {
  while (*line == ' ' || *line == '\t') ++line;
  if (*line == '\0' || *line == ';') {
    Serial.println(F("ok"));
    return;
  }

  if (strcmp(line, "$H") == 0) {
    homeMachine();
    if (machineHomed && !alarmActive) Serial.println(F("ok"));
    else Serial.println(F("error: homing failed"));
    return;
  }
  if (strcmp(line, "$X") == 0) {
    if (estopTriggered() || xHomeTriggered() || yHomeTriggered()) {
      printError(F("release E-stop and ensure home switches are closed first"));
      return;
    }
    alarmActive = false;
    machineHomed = false;
    driversEnable(true);
    Serial.println(F("ok"));
    return;
  }
  if (strcmp(line, "M3") == 0) {
    if (alarmActive) { printError(F("alarm active")); return; }
    lowerPen();
    delay(PEN_SETTLE_MS);
    Serial.println(F("ok"));
    return;
  }
  if (strcmp(line, "M5") == 0) {
    raisePen();
    delay(PEN_SETTLE_MS);
    Serial.println(F("ok"));
    return;
  }
  if (strcmp(line, "M114") == 0) {
    reportPosition();
    Serial.println(F("ok"));
    return;
  }
  if (strcmp(line, "G21") == 0 || strcmp(line, "G90") == 0) {
    Serial.println(F("ok"));
    return;
  }

  char *save = nullptr;
  char *token = strtok_r(line, " \t", &save);
  if (!token) { Serial.println(F("ok")); return; }
  const bool rapid = strcmp(token, "G0") == 0;
  const bool linear = strcmp(token, "G1") == 0;
  if (!rapid && !linear) { printError(F("unsupported command")); return; }

  float x = xAxis.currentPosition() / X_STEPS_PER_MM;
  float y = yAxis.currentPosition() / Y_STEPS_PER_MM;
  float feed = rapid ? MAX_FEED_MM_MIN : modalFeedMmMin;

  while ((token = strtok_r(nullptr, " \t", &save)) != nullptr) {
    float value;
    if (token[0] == 'X') {
      if (!parseFloatStrict(token + 1, value)) { printError(F("invalid X value")); return; }
      x = value;
    } else if (token[0] == 'Y') {
      if (!parseFloatStrict(token + 1, value)) { printError(F("invalid Y value")); return; }
      y = value;
    } else if (token[0] == 'F') {
      if (!parseFloatStrict(token + 1, value)) { printError(F("invalid feed value")); return; }
      feed = value;
    } else {
      printError(F("unsupported parameter"));
      return;
    }
  }

  if (moveLine(x, y, feed)) {
    if (linear) modalFeedMmMin = feed;
    Serial.println(F("ok"));
  } else {
    Serial.println(F("error: move rejected or interrupted"));
  }
}

void setup() {
  pinMode(DRIVER_ENABLE_PIN, OUTPUT);
  pinMode(X_HOME_PIN, INPUT_PULLUP);
  pinMode(Y_HOME_PIN, INPUT_PULLUP);
  pinMode(ESTOP_PIN, INPUT_PULLUP);
  driversEnable(false);

  penServo.attach(PEN_PIN);
  raisePen();

  xAxis.setMinPulseWidth(3);
  yAxis.setMinPulseWidth(3);
  xAxis.setMaxSpeed(MAX_AXIS_SPEED_STEPS_S);
  yAxis.setMaxSpeed(MAX_AXIS_SPEED_STEPS_S);
  xAxis.setAcceleration(PATH_ACCEL_MM_S2 * X_STEPS_PER_MM);
  yAxis.setAcceleration(PATH_ACCEL_MM_S2 * Y_STEPS_PER_MM);

  Serial.begin(115200);
  delay(300);
  Serial.println(F("CNC_CATASTROPHE_READY"));
}

void loop() {
  if (!alarmActive && estopTriggered()) enterAlarm(F("E-stop circuit open"));

  while (Serial.available() > 0) {
    const char incoming = (char)Serial.read();
    if (incoming == '\n' || incoming == '\r') {
      if (serialLength > 0) {
        serialBuffer[serialLength] = '\0';
        processCommand(serialBuffer);
        serialLength = 0;
      }
    } else if (serialLength < SERIAL_BUFFER_SIZE - 1) {
      serialBuffer[serialLength++] = incoming;
    } else {
      serialLength = 0;
      printError(F("command too long"));
    }
  }
}
