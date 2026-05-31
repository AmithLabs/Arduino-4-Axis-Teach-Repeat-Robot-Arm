/*
  ---------------------------------------------------------
  Project         : 4 Axis Teach & Repeat Robot Arm V1
  Youtube Channel : @AmithLabs
  Author          : Amith Wijekoon
  ---------------------------------------------------------

  FINAL WORKING VERSION

  Features:
  - 4 Axis Master-Slave Robot Control
  - Teach & Repeat Function
  - EEPROM Position Memory
  - 20 Position Storage
  - Continuous Playback Loop
  - WS2812 RGB Status Indicator
  - Smooth Servo Motion Control
  - Analog Noise Filtering
  - Program Reset Function

  Hardware:
  Board        : Arduino UNO
  RGB LED      : WS2812 (1 LED) Pin D5

  Servo Pins:
  Grip : D3
  J1   : D9
  J2   : D10
  J3   : D11

  Control Pots:
  Grip : A5
  J1   : A4
  J2   : A3
  J3   : A2

  Buttons:
  BTN1 : Teach / Save Position , D6
  BTN2 : Play / Stop / Reset , D7
  ---------------------------------------------------------
*/

#include <Servo.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>

// =========================================
// PIN DEFINITIONS
// =========================================

#define BTN1 6
#define BTN2 7
#define LED_PIN 5

#define GRIP_SERVO 3
#define J1_SERVO 9
#define J2_SERVO 10
#define J3_SERVO 11

// =========================================
// RGB LED
// =========================================

Adafruit_NeoPixel pixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

// =========================================
// SERVO OBJECTS
// =========================================

Servo gripServo;
Servo j1Servo;
Servo j2Servo;
Servo j3Servo;

// =========================================
// POSITION STRUCTURE
// =========================================

struct Position {
  byte grip;
  byte j1;
  byte j2;
  byte j3;
};

Position positions[20];

int pointCount = 0;

// =========================================
// SYSTEM FLAGS
// =========================================

bool teaching = false;
bool playing = false;
bool programReady = false;

// =========================================
// CURRENT POSITIONS
// =========================================

int gripCurrent, j1Current, j2Current, j3Current;

// =========================================
// LED COLOR FUNCTION
// =========================================

void setColor(int r, int g, int b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

// =========================================
// EEPROM FUNCTIONS
// =========================================

void saveToEEPROM() {
  EEPROM.put(0, pointCount);
  EEPROM.put(2, positions);
}

void loadFromEEPROM() {
  EEPROM.get(0, pointCount);
  EEPROM.get(2, positions);

  if (pointCount > 0 && pointCount <= 20)
    programReady = true;
}

void clearProgram() {
  pointCount = 0;
  programReady = false;
  playing = false;
  teaching = false;

  EEPROM.put(0, 0);

  setColor(0,0,255);
}

// =========================================
// ANALOG NOISE FILTER
// =========================================

// Average 10 readings for smoother signal
int smoothAnalogRead(int pin) {

  long total = 0;

  for(int i = 0; i < 10; i++) {
    total += analogRead(pin);
    delayMicroseconds(300);
  }

  return total / 10;
}

// =========================================
// SMOOTH SERVO MOVEMENT
// =========================================

void smoothMove(int &current, int target, int maxStep) {

  int error = target - current;
  int distance = abs(error);

  if (distance > 0) {

    int step;

    if (distance > 40)
      step = maxStep;
    else if (distance > 20)
      step = maxStep - 2;
    else if (distance > 8)
      step = 2;
    else
      step = 1;

    if (step < 1) step = 1;

    if (error > 0)
      current += step;
    else
      current -= step;
  }
}

// =========================================
// READ POTENTIOMETERS
// =========================================

void readTargets(int &g, int &j1, int &j2, int &j3) {

  g  = constrain(map(smoothAnalogRead(A5), 300, 0, 0, 60), 0, 60);

  j1 = constrain(map(smoothAnalogRead(A4), 1023, 523, 40, 180), 40, 180);

  j2 = constrain(map(smoothAnalogRead(A3), 780, 70, 25, 180), 25, 180);

  j3 = constrain(map(smoothAnalogRead(A2), 740, 0, 0, 170), 0, 170);
}

// =========================================
// MOVE TO TARGET
// =========================================

bool moveToTarget(Position target) {

  smoothMove(gripCurrent, target.grip, 3);
  smoothMove(j1Current, target.j1, 5);
  smoothMove(j2Current, target.j2, 5);
  smoothMove(j3Current, target.j3, 5);

  gripServo.write(gripCurrent);
  j1Servo.write(j1Current);
  j2Servo.write(j2Current);
  j3Servo.write(j3Current);

  return (
    abs(gripCurrent - target.grip) <= 1 &&
    abs(j1Current - target.j1) <= 1 &&
    abs(j2Current - target.j2) <= 1 &&
    abs(j3Current - target.j3) <= 1
  );
}

// =========================================
// PLAY PROGRAM
// =========================================

void playProgram() {

  int currentPoint = 0;

  while (playing) {

    while (!moveToTarget(positions[currentPoint]) && playing) {

      if (digitalRead(BTN2) == HIGH) {

        delay(200);
        playing = false;
        setColor(255,255,255);

        while(digitalRead(BTN2)==HIGH);
      }

      delay(5);
    }

    unsigned long holdStart = millis();

    while (millis() - holdStart < 1000 && playing) {

      gripServo.write(gripCurrent);
      j1Servo.write(j1Current);
      j2Servo.write(j2Current);
      j3Servo.write(j3Current);

      if (digitalRead(BTN2) == HIGH) {

        delay(200);
        playing = false;
        setColor(255,255,255);

        while(digitalRead(BTN2)==HIGH);
      }

      delay(10);
    }

    currentPoint++;

    if (currentPoint >= pointCount)
      currentPoint = 0;
  }
}

// =========================================
// SETUP
// =========================================

void setup() {

  Serial.begin(9600);

  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);

  pixel.begin();

  readTargets(gripCurrent, j1Current, j2Current, j3Current);

  gripServo.attach(GRIP_SERVO);
  j1Servo.attach(J1_SERVO);
  j2Servo.attach(J2_SERVO);
  j3Servo.attach(J3_SERVO);

  gripServo.write(gripCurrent);
  j1Servo.write(j1Current);
  j2Servo.write(j2Current);
  j3Servo.write(j3Current);

  loadFromEEPROM();

  setColor(255,255,255);
}

// =========================================
// MAIN LOOP
// =========================================

void loop() {

  // BUTTON 1 - TEACH / SAVE

  if (digitalRead(BTN1) == HIGH && !playing) {

    delay(200);

    if (!teaching) {

      teaching = true;
      setColor(255,0,0);

    } else {

      if (pointCount < 20) {

        readTargets(gripCurrent, j1Current, j2Current, j3Current);

        positions[pointCount].grip = gripCurrent;
        positions[pointCount].j1 = j1Current;
        positions[pointCount].j2 = j2Current;
        positions[pointCount].j3 = j3Current;

        pointCount++;

        saveToEEPROM();

        programReady = true;

        setColor(0,0,255);
      }

      teaching = false;
    }

    while(digitalRead(BTN1)==HIGH);
  }

  // BUTTON 2 - PLAY / RESET

  if (digitalRead(BTN2) == HIGH) {

    unsigned long pressTime = millis();

    while(digitalRead(BTN2)==HIGH) {

      if (millis() - pressTime > 2000) {

        clearProgram();

        while(digitalRead(BTN2)==HIGH);
        return;
      }
    }

    delay(200);

    if (programReady) {

      if (!playing) {

        playing = true;
        setColor(0,255,0);

        playProgram();

      } else {

        playing = false;
        setColor(255,255,255);
      }
    }
  }

  // LIVE FOLLOW MODE

  if (teaching) {

    int g,j1,j2,j3;

    readTargets(g,j1,j2,j3);

    smoothMove(gripCurrent,g,3);
    smoothMove(j1Current,j1,5);
    smoothMove(j2Current,j2,5);
    smoothMove(j3Current,j3,5);

    gripServo.write(gripCurrent);
    j1Servo.write(j1Current);
    j2Servo.write(j2Current);
    j3Servo.write(j3Current);

    delay(5);
  }
}