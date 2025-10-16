#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Servo.h>

#define DIR1 2
#define STEP1 3

#define DIR2 4
#define STEP2 5

#define StepPin 6

#define Sel A2
#define Horz A1
#define Vert A0

int pos=0;
Servo servo1;
AccelStepper stepper1(AccelStepper::DRIVER, STEP1, DIR1);
AccelStepper stepper2(AccelStepper::DRIVER, STEP2, DIR2);
MultiStepper steppers;

int read = LOW;
int lastRead = HIGH;
int swi = 0;

void setup() {
  servo1.attach(StepPin);
  Serial.begin(9600);
  steppers.addStepper(stepper1);
  steppers.addStepper(stepper2);
  pinMode(Sel, INPUT_PULLUP);
  pinMode(Horz, INPUT);
  pinMode(Vert, INPUT);
}

void loop() {
  read = digitalRead(Sel);
  if(read == LOW && lastRead == HIGH){
    swi = !swi;
  }
  if (swi == 0){
    servo1.write(90);
  }
  else{
    servo1.write(0);
  }
  int xDir = analogRead(Horz);
  int yDir = analogRead(Vert);

  // Center to 0
  xDir -= 512;
  yDir -= 512;

  int deadZone = 100;
  if (abs(xDir) < deadZone) xDir = 0;
  if (abs(yDir) < deadZone) yDir = 0;

  if(xDir > 0)
    stepper1.setSpeed(200);
  else
    stepper1.setSpeed(-200);

  if(yDir > 0)
    stepper2.setSpeed(200);
  else
    stepper2.setSpeed(-200);
  
  stepperX.runSpeed();
  stepperY.runSpeed();

  lastRead = read;
}
