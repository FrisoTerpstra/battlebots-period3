// ==========================================
// MAZE LINE FOLLOWER — CASCADE VERSION
// ==========================================

#include <Adafruit_NeoPixel.h> // Libraries
#include <Servo.h>

// --- NEOPIXEL ---
#define LED_PIN  7 // what pin is used
#define NUM_LEDS 4 // number of leds
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800); // needed for neopixels

// --- GRIPPER / ULTRASONIC ---
const int GRIPPER_PIN          = 10;
const int TRIG_PIN             = 11;
const int ECHO_PIN             = 12;
const int OBJECT_DISTANCE_CM   = 50; // treshold from what distance it sees the cone and picks it up

Servo gripper;
const int GRIPPER_OPEN_US   = 1800; // pusle lenght
const int GRIPPER_CLOSED_US = 1000;

const unsigned long START_FORWARD_1 = 1400; // values for the beginning of race
const unsigned long START_GRAB_WAIT = 800;
const unsigned long START_TURN_LEFT = 600;
const unsigned long START_FORWARD_2 = 1000;

// --- MOTOR PIN DEFINITIONS ---
const int MOTOR_A_1 = 3;
const int MOTOR_A_2 = 5;
const int MOTOR_B_1 = 6;
const int MOTOR_B_2 = 9;

// --- SENSOR MAPPING ---
const int SENSOR_PINS[] = {A0, A1, A2, A3, A4, A5, A6, A7}; // pins for sensors start at 0 because it is an array
int val[8];

// --- SENSITIVITY CALIBRATION ---
int threshold = 850;

// --- RACE SPEED CALIBRATION ---
int leftSpeed  = 255;
int rightSpeed = 247;

// --- CORRECTION LEVELS ---
int microCorrection  = 20;
int gentleCorrection = 40;
int mediumCorrection = 45;

// --- DIRECTION MEMORY ---
int lastDirection = 0;

// --- RACE START TIME ---
unsigned long raceStartTime = 0;

// --- MAZE TIMING ---
const unsigned long CROSSING_TIME       = 200;
const unsigned long TURN_90_TIME        = 500; // how long it rotates in a 90 and 180 degree turn (should be calibrated correct)
const unsigned long TURN_180_TIME       = 800;
const unsigned long ignoreFinishTime    = 1000;
const unsigned long FINISH_CONFIRM_TIME = 75;

// --- DEAD END COUNTER ---
int deadEndCount = 0;
const int DEAD_END_LIMIT = 10;

// --- FINISH DETECTED FLAG ---
bool finishDetected = false;

// --- OBJECT AVOIDANCE ---
bool mazeStarted = false;
const int AVOID_DISTANCE_CM = 15; // distance to see an object and avoid it

// --- STATE MACHINE ---
enum State {
  FOLLOW,
  JUNCTION,
  TURNING,
  SEARCH,
  RECOVER,
  FINISH,
  BACKUP,
  COMPLETED
};
State state = FOLLOW;
unsigned long stateTimer = 0;
int pendingTurn = 0;

// --- TIMING ---
unsigned long previousLoopTime = 0;
const long LOOP_INTERVAL = 10;

// -------------------------------------------------------
// ULTRASONIC
// -------------------------------------------------------

long getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // calculate distance
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

bool obstacleAhead() {
  if (!mazeStarted) return false;

  long distance = getDistanceCM();
  Serial.print("Obstacle distance: ");
  Serial.println(distance);

  return (distance > 0 && distance <= AVOID_DISTANCE_CM);
}

// -------------------------------------------------------
// GRIPPER
// -------------------------------------------------------

void openGripper() {
  gripper.writeMicroseconds(GRIPPER_OPEN_US);
}

void closeGripper() {
  gripper.writeMicroseconds(GRIPPER_CLOSED_US);
}

// -------------------------------------------------------
// SETUP
// -------------------------------------------------------

void setup() {
  pinMode(MOTOR_A_1, OUTPUT);
  pinMode(MOTOR_A_2, OUTPUT);
  pinMode(MOTOR_B_1, OUTPUT);
  pinMode(MOTOR_B_2, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  for (int i = 0; i < 8; i++) {
    pinMode(SENSOR_PINS[i], INPUT);
  }

  pixels.begin(); // to make the pixels work
  pixels.clear();
  pixels.show();

  Serial.begin(9600);

  gripper.attach(GRIPPER_PIN);
  openGripper();

  stopMotors();

  while (true) {
    long distance = getDistanceCM();
    if (distance <= OBJECT_DISTANCE_CM) { // check if something is infront of it
      break;
    }
    delay(50);
  }

  delay(3000); // wait for the other robot to leave then do the beginning code
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

  raceStartTime = millis();
  mazeStarted = true;
}

// -------------------------------------------------------
// LED FUNCTIONS
// -------------------------------------------------------

void showForwardLights() { // for the neopixels
  pixels.clear();
  pixels.setPixelColor(2, pixels.Color(100, 100, 100));
  pixels.setPixelColor(3, pixels.Color(100, 100, 100));
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

void showReverseLights() {
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(100, 0, 0));
  pixels.setPixelColor(1, pixels.Color(100, 0, 0));
  pixels.show();
}

void showCompletedLights() {
  pixels.fill(pixels.Color(0, 255, 0));
  pixels.show();
}

void showStopLights() {
  pixels.clear();
  pixels.show();
}

// -------------------------------------------------------
// SENSOR HELPERS
// -------------------------------------------------------

bool on(int i) {
  return val[i] > threshold;
}

int countActive() {
  int n = 0;
  for (int i = 0; i < 8; i++) {
    if (on(i)) n++;
  }
  return n;
}

bool centerActive() { // check the sensors to see which ones are seen and react on that
  return on(2) || on(3) || on(4) || on(5);
}

bool leftOpen() {
  return (on(6) || on(7)) && centerActive();
}

bool rightOpen() {
  return (on(0) || on(1)) && centerActive();
}

bool atFinishPattern() {
  return on(0) && on(7) && on(3) && on(4);
}

bool atJunction() {
  return (on(0) || on(7)) && centerActive() && !atFinishPattern();
}

bool deadEnd() {
  return countActive() == 0;
}

int chooseTurn() {
  if (leftOpen())     return -1;
  if (centerActive()) return  0;
  if (rightOpen())    return  1;
  return 2;
}

// -------------------------------------------------------
// CASCADE LINE FOLLOW
// -------------------------------------------------------

void followLine() {
  if (val[3] > threshold && val[4] > threshold) {
    showForwardLights();
    forward();
  }
  else if (val[3] > threshold) {
    showLeftLights();
    lastDirection = -1;
    analogWrite(MOTOR_A_1, 0);
    analogWrite(MOTOR_A_2, leftSpeed - microCorrection);
    analogWrite(MOTOR_B_1, rightSpeed);
    analogWrite(MOTOR_B_2, 0);
  }
  else if (val[4] > threshold) {
    showRightLights();
    lastDirection = 1;
    analogWrite(MOTOR_A_1, 0);
    analogWrite(MOTOR_A_2, leftSpeed);
    analogWrite(MOTOR_B_1, rightSpeed - microCorrection);
    analogWrite(MOTOR_B_2, 0);
  }
  else if (val[2] > threshold) {
    showRightLights();
    lastDirection = 1;
    turnRightGentle();
  }
  else if (val[5] > threshold) {
    showLeftLights();
    lastDirection = -1;
    turnLeftGentle();
  }
  else if (val[1] > threshold) {
    showRightLights();
    lastDirection = 1;
    turnRightMedium();
  }
  else if (val[6] > threshold) {
    showLeftLights();
    lastDirection = -1;
    turnLeftMedium();
  }
  else if (val[0] > threshold) {
    showRightLights();
    lastDirection = 1;
    rotateMotorsRight();
  }
  else if (val[7] > threshold) {
    showLeftLights();
    lastDirection = -1;
    rotateMotorsLeft();
  }
  else {
    if (lastDirection == 1) {
      showRightLights();
      rotateMotorsRight();
    } else if (lastDirection == -1) {
      showLeftLights();
      rotateMotorsLeft();
    } else {
      showStopLights();
      stopMotors();
    }
  }
}

// -------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------

void loop() { // change states to givin situations
  unsigned long currentTime = millis(); 

  if (currentTime - previousLoopTime >= LOOP_INTERVAL) {
    previousLoopTime = currentTime;

    for (int i = 0; i < 8; i++) {
      val[i] = analogRead(SENSOR_PINS[i]);
    }

    switch (state) {

      case FOLLOW:
        if (obstacleAhead()) {
          stopMotors();
          pendingTurn = 2;
          state = TURNING;
          stateTimer = currentTime;
          break;
        }
        else if (atFinishPattern()
            && !atJunction()
            && (currentTime - raceStartTime > ignoreFinishTime)) {
          if (!finishDetected) {
            finishDetected = true;
            stateTimer     = currentTime;
          }
          if (currentTime - stateTimer >= FINISH_CONFIRM_TIME) {
            state          = FINISH;
            stateTimer     = currentTime;
            finishDetected = false;
          }
          showForwardLights();
          forward();
        }
        else {
          finishDetected = false;
          if (atJunction()) {
            deadEndCount = 0;
            pendingTurn  = chooseTurn();
            if (pendingTurn == 0) {
              followLine();
            } else {
              showForwardLights();
              forward();
              state      = JUNCTION;
              stateTimer = currentTime;
            }
          }
          else if (deadEnd()) {
            deadEndCount++;
            if (deadEndCount >= DEAD_END_LIMIT) {
              deadEndCount = 0;
              state = RECOVER;
            } else {
              followLine();
            }
          }
          else {
            deadEndCount = 0;
            followLine();
          }
        }
        break;

      case JUNCTION:
        showForwardLights();
        forward();
        if (currentTime - stateTimer >= CROSSING_TIME) {
          state      = TURNING;
          stateTimer = currentTime;
        }
        break;

      case TURNING: {
        unsigned long needed;
        if (pendingTurn == 2) {
          needed = TURN_180_TIME;
        } else {
          needed = TURN_90_TIME;
        }
        if (pendingTurn == -1) {
          showLeftLights();
          rotateMotorsLeft();
          lastDirection = -1;
        } else {
          showRightLights();
          rotateMotorsRight();
          lastDirection = 1;
        }
        if (currentTime - stateTimer >= needed) {
          state = SEARCH;
        }
        break;
      }

      case SEARCH:
        if (pendingTurn == -1) {
          showLeftLights();
          rotateMotorsLeft();
        } else {
          showRightLights();
          rotateMotorsRight();
        }
        if (centerActive()) state = FOLLOW;
        break;

      case RECOVER:
        if (lastDirection == 1) {
          showRightLights();
          rotateMotorsRight();
        } else if (lastDirection == -1) {
          showLeftLights();
          rotateMotorsLeft();
        } else {
          showStopLights();
          stopMotors();
        }
        if (!deadEnd()) state = FOLLOW;
        break;

      case FINISH:
        showForwardLights();
        forward();
        if (countActive() <= 2) {
          stopMotors();
          showReverseLights();
          stateTimer = currentTime;
          state      = BACKUP;
        }
        break;

      case BACKUP:
        showReverseLights();
        if (currentTime - stateTimer < 700) {
          reverseMotors();
        }
        else if (currentTime - stateTimer < 1200) {
          stopMotors();
          openGripper();
        }
        else if (currentTime - stateTimer < 3000) {
          reverseMotors();
        }
        else {
          state = COMPLETED;
        }
        break;

      case COMPLETED:
        stopMotors();
        showCompletedLights();
        break;
    }
  }
}

// -------------------------------------------------------
// MOTOR FUNCTIONS
// -------------------------------------------------------

void forward() { // simple functions to clear up code
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, leftSpeed);
  analogWrite(MOTOR_B_1, rightSpeed);
  analogWrite(MOTOR_B_2, 0);
}

void turnLeftGentle() {
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, 255 - gentleCorrection);
  analogWrite(MOTOR_B_1, 255);
  analogWrite(MOTOR_B_2, 0);
}

void turnRightGentle() {
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, 255);
  analogWrite(MOTOR_B_1, 255 - gentleCorrection);
  analogWrite(MOTOR_B_2, 0);
}

void turnLeftMedium() {
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, 255 - mediumCorrection);
  analogWrite(MOTOR_B_1, 255);
  analogWrite(MOTOR_B_2, 0);
}

void turnRightMedium() {
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, 255);
  analogWrite(MOTOR_B_1, 255 - mediumCorrection);
  analogWrite(MOTOR_B_2, 0);
}

void rotateMotorsLeft() {
  analogWrite(MOTOR_A_1, 255);
  analogWrite(MOTOR_A_2, 0);
  analogWrite(MOTOR_B_1, 255);
  analogWrite(MOTOR_B_2, 0);
}

void rotateMotorsRight() {
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, 255);
  analogWrite(MOTOR_B_1, 0);
  analogWrite(MOTOR_B_2, 255);
}

void reverseMotors() {
  analogWrite(MOTOR_A_1, 255);
  analogWrite(MOTOR_A_2, 0);
  analogWrite(MOTOR_B_1, 0);
  analogWrite(MOTOR_B_2, 255);
}

void stopMotors() {
  analogWrite(MOTOR_A_1, 0);
  analogWrite(MOTOR_A_2, 0);
  analogWrite(MOTOR_B_1, 0);
  analogWrite(MOTOR_B_2, 0);
}
