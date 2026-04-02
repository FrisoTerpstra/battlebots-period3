#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <Servo.h>

#define LED_PIN 7
#define NUM_LEDS 4

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

SoftwareSerial bt(2, 4);

const int GRIPPER_PIN = 10;
const int TRIG_PIN = 11;
const int ECHO_PIN = 12;
const int OBJECT_DISTANCE_CM = 40;

Servo gripper;

const int GRIPPER_OPEN_US = 1800;
const int GRIPPER_CLOSED_US = 1100;

const unsigned long START_FORWARD_1 = 1600;
const unsigned long START_GRAB_WAIT = 800;
const unsigned long START_TURN_LEFT = 575;
const unsigned long START_FORWARD_2 = 1200;
const unsigned long WAIT_FOR_ROBOT_LEAVES = 3000;

// --- MOTOR PIN DEFINITIONS ---
const int MOTOR_A_1 = 3;
const int MOTOR_A_2 = 5;
const int MOTOR_B_1 = 6;
const int MOTOR_B_2 = 9;

// --- SENSOR MAPPING ---
const int SENSOR_PINS[] = {A0, A1, A2, A3, A4, A5, A6, A7};
int val[8];

// --- SENSITIVITY CALIBRATION ---
int threshold = 900;

// --- RACE SPEED CALIBRATION ---
int leftSpeed  = 240;
int rightSpeed = 222;

// --- DIRECTION MEMORY ---
int lastDirection = 0;

// --- TIMING ---
unsigned long previousLoopTime = 0;
const long LOOP_INTERVAL = 10;

// --- MAZE TIMING ---
const unsigned long CROSSING_TIME = 200;
const unsigned long TURN_90_TIME  = 600;
const unsigned long TURN_180_TIME = 1000;

// --- DEAD END COUNTER ---
int deadEndCount = 0;
const int DEAD_END_LIMIT = 30;

// --- STATE MACHINE ---
enum State { FOLLOW, CROSSING, TURNING, SEARCH, RECOVER };
State state = FOLLOW;
unsigned long stateTimer  = 0;
int pendingTurn = 0;

// -------------------------------------------------------
// SENSOR HELPERS
// -------------------------------------------------------

void readSensors() {
  for (int i = 0; i < 8; i++) {
    val[i] = analogRead(SENSOR_PINS[i]);
  }
}

bool on(int i) {
  return val[i] > threshold;
}

bool anyActive(int a, int b) {
  for (int i = a; i <= b; i++) {
    if (on(i)) {
      return true;
    }
  }
  return false;
}

int countActive() {
  int n = 0;
  for (int i = 0; i < 8; i++) {
    if (on(i)) {
      n++;
    }
  }
  return n;
}

bool rightOpen() {
  return on(0) || on(1);
}

bool leftOpen() {
  return on(7) && on(6) && on(5);
}

bool straightOpen() {
  return anyActive(2, 5);
}

bool atIntersection() {
  bool centreActive = anyActive(2, 5);
  bool edgeActive   = on(0) || on(7);
  return centreActive && edgeActive;
}

bool deadEnd() {
  return countActive() == 0;
}

// Priority: left > straight > right > U-turn
int chooseTurn() {
  if (leftOpen()) {
    return -1;
  } else if (straightOpen()) {
    return 0;
  } else if (rightOpen()) {
    return 1;
  } else {
    return 2;
  }
}

// -------------------------------------------------------
// DEBUG OUTPUT
// -------------------------------------------------------

void debugPrint() {
  String state_name;
  switch (state) {
    case FOLLOW:   state_name = "FOLLOW";   break;
    case CROSSING: state_name = "CROSSING"; break;
    case TURNING:  state_name = "TURNING";  break;
    case SEARCH:   state_name = "SEARCH";   break;
    case RECOVER:  state_name = "RECOVER";  break;
  }
  String line = String(millis()) + "," + state_name + ",";
  for (int i = 0; i < 8; i++) {
    line += String(val[i]);
    if (i < 7) line += ",";
  }
  line += "," + String(leftSpeed) + "," + String(rightSpeed);
  Serial.println(line);
  bt.println(line);
}

// -------------------------------------------------------
// START HELPERS
// -------------------------------------------------------

long getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999;
  return duration * 0.04 / 2;
}

void openGripper() {
  gripper.writeMicroseconds(GRIPPER_OPEN_US);
}

void closeGripper() {
  gripper.writeMicroseconds(GRIPPER_CLOSED_US);
}

// -------------------------------------------------------
// LINE FOLLOWING
// -------------------------------------------------------

void followLine() {
  if (val[3] > threshold || val[4] > threshold) {
    forward();
  }
  else if (val[2] > threshold) {
    lastDirection = 1;
    turnRightGentle();
  }
  else if (val[5] > threshold) {
    lastDirection = -1;
    turnLeftGentle();
  }
  else if (val[1] > threshold) {
    lastDirection = 1;
    turnRightMedium();
  }
  else if (val[6] > threshold) {
    lastDirection = -1;
    turnLeftMedium();
  }
  else if (val[0] > threshold) {
    lastDirection = 1;
    rotateMotorsRight();
  }
  else if (val[7] > threshold) {
    lastDirection = -1;
    rotateMotorsLeft();
  }
  else {
    if (lastDirection == 1) {
      rotateMotorsRight();
    } else if (lastDirection == -1) {
      rotateMotorsLeft();
    } else {
      stopMotors();
    }
  }
}

// -------------------------------------------------------
// SETUP
// -------------------------------------------------------

void setup() {
  pinMode(MOTOR_A_1, OUTPUT); pinMode(MOTOR_A_2, OUTPUT);
  pinMode(MOTOR_B_1, OUTPUT); pinMode(MOTOR_B_2, OUTPUT);
  for (int i = 0; i < 8; i++) pinMode(SENSOR_PINS[i], INPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.begin(9600);
  bt.begin(9600);

  pixels.begin();
  pixels.clear();
  pixels.show();

  gripper.attach(GRIPPER_PIN);
  openGripper();

  stopMotors();

  while (true) {
    long distance = getDistanceCM();
    if (distance <= OBJECT_DISTANCE_CM) {
      break;
    }
    delay(50);
  }

  delay(WAIT_FOR_ROBOT_LEAVES);
  forward();
  delay(START_FORWARD_1);

  stopMotors();
  closeGripper();
  delay(START_GRAB_WAIT);

  rotateMotorsLeft();
  delay(START_TURN_LEFT);

  forward();
  delay(START_FORWARD_2);

  stopMotors();

  Serial.println("System Online");
}

// -------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - previousLoopTime >= LOOP_INTERVAL) {
    previousLoopTime = currentTime;

    readSensors();
    debugPrint();

    switch (state) {

      case FOLLOW:
        if (atIntersection()) {
          deadEndCount = 0;
          pendingTurn  = chooseTurn();
          if (pendingTurn == 0) {
            followLine();
          } else {
            forward();
            state      = CROSSING;
            stateTimer = currentTime;
          }
        } else if (deadEnd()) {
          deadEndCount++;
          if (deadEndCount >= DEAD_END_LIMIT) {
            deadEndCount = 0;
            state = RECOVER;
          } else {
            followLine();
          }
        } else {
          deadEndCount = 0;
          followLine();
        }
        break;

      case CROSSING:
        forward();
        if (currentTime - stateTimer >= CROSSING_TIME) {
          state      = TURNING;
          stateTimer = currentTime;
        }
        break;

      case TURNING: {
        unsigned long needed = (pendingTurn == 2) ? TURN_180_TIME : TURN_90_TIME;
        if (pendingTurn == -1) { rotateMotorsLeft();  lastDirection = -1; }
        else                   { rotateMotorsRight(); lastDirection =  1; }
        if (currentTime - stateTimer >= needed) state = SEARCH;
        break;
      }

      case SEARCH:
        if (pendingTurn == -1) rotateMotorsLeft();
        else                   rotateMotorsRight();
        if (anyActive(2, 5)) state = FOLLOW;
        break;

      case RECOVER:
        if      (lastDirection ==  1) rotateMotorsRight();
        else if (lastDirection == -1) rotateMotorsLeft();
        else                          stopMotors();
        if (!deadEnd()) state = FOLLOW;
        break;
    }
  }
}

// -------------------------------------------------------
// MOTOR FUNCTIONS
// -------------------------------------------------------

void forward() {
  showForwardLights();
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, leftSpeed);
  analogWrite(MOTOR_B_1, rightSpeed);
  analogWrite(MOTOR_B_2, 0);
}

void turnLeftGentle() {
  showLeftLights();
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, leftSpeed - 120);
  analogWrite(MOTOR_B_1, rightSpeed);
  analogWrite(MOTOR_B_2, 0);
}

void turnRightGentle() {
  showRightLights();
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, leftSpeed);
  analogWrite(MOTOR_B_1, rightSpeed - 120);
  analogWrite(MOTOR_B_2, 0);
}

void turnLeftMedium() {
  showLeftLights();
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, 0);
  analogWrite(MOTOR_B_1, rightSpeed);
  analogWrite(MOTOR_B_2, 0);
}

void turnRightMedium() {
  showRightLights();
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, leftSpeed);
  analogWrite(MOTOR_B_1, 0);
  analogWrite(MOTOR_B_2, 0);
}

void rotateMotorsLeft() {
  showLeftLights();
  analogWrite(MOTOR_A_1, leftSpeed);
  analogWrite(MOTOR_A_2, 0);
  analogWrite(MOTOR_B_1, rightSpeed);
  analogWrite(MOTOR_B_2, 0);
}

void rotateMotorsRight() {
  showRightLights();
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, leftSpeed);
  analogWrite(MOTOR_B_1, 0);
  analogWrite(MOTOR_B_2, rightSpeed);
}

void stopMotors() {
  showBackwardLights();
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, 0);
  analogWrite(MOTOR_B_1, 0);
  analogWrite(MOTOR_B_2, 0);
}

// -------------------------------------------------------
// LIGHT FUNCTIONS
// -------------------------------------------------------

void showForwardLights() {
  pixels.clear();
  pixels.setPixelColor(2, pixels.Color(100, 100, 100));
  pixels.setPixelColor(3, pixels.Color(100, 100, 100));
  pixels.show();
}

void showBackwardLights() {
  pixels.clear();
  pixels.setPixelColor(1, pixels.Color(100, 0, 0));
  pixels.setPixelColor(0, pixels.Color(100, 0, 0));
  pixels.show();
}

void showLeftLights() {
  pixels.clear();
  pixels.setPixelColor(3, pixels.Color(65, 40, 0));
  pixels.show();
}

void showRightLights() {
  pixels.clear();
  pixels.setPixelColor(2, pixels.Color(65, 40, 0));
  pixels.show();
}
