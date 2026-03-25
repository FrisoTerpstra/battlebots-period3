#include <SoftwareSerial.h>
SoftwareSerial bt(2, 4);

// --- MOTOR PIN DEFINITIONS ---
const int MOTOR_A_1 = 3;
const int MOTOR_A_2 = 5;
const int MOTOR_B_1 = 6;
const int MOTOR_B_2 = 9;

// --- SENSOR MAPPING ---
const int SENSOR_PINS[] = {A0, A1, A2, A3, A4, A5, A6, A7};
int val[8];

// --- SENSITIVITY CALIBRATION ---
int threshold = 800;

// --- RACE SPEED CALIBRATION ---
int leftSpeed  = 255;
int rightSpeed = 250;

// --- DIRECTION MEMORY ---
int lastDirection = 0;

// --- TIMING ---
unsigned long previousLoopTime = 0;
const long LOOP_INTERVAL = 10;

// --- MAZE TIMING ---
const unsigned long CROSSING_TIME = 200;
const unsigned long TURN_90_TIME  = 400;
const unsigned long TURN_180_TIME = 800;

// --- DEAD END COUNTER ---
int deadEndCount = 0;
const int DEAD_END_LIMIT = 30;

// --- STATE MACHINE ---
enum State { FOLLOW, CROSSING, TURNING, SEARCH, RECOVER };
State state = FOLLOW;
unsigned long stateTimer  = 0;
int           pendingTurn = 0;

// -------------------------------------------------------
// SENSOR HELPERS
// -------------------------------------------------------

void readSensors() {
  for (int i = 0; i < 8; i++) val[i] = analogRead(SENSOR_PINS[i]);
}

bool on(int i) { return val[i] > threshold; }

bool anyActive(int a, int b) {
  for (int i = a; i <= b; i++) if (on(i)) return true;
  return false;
}
int countActive() {
  int n = 0;
  for (int i = 0; i < 8; i++) if (on(i)) n++;
  return n;
}

// Junction only fires when outermost edge AND centre sensors active together
bool rightOpen()    { return on(0); }
bool leftOpen()     { return on(7); }
bool straightOpen() { return anyActive(2, 5); }

bool atIntersection() {
  bool centreActive = anyActive(2, 5);
  bool edgeActive   = on(0) || on(7);
  return centreActive && edgeActive;
}

bool deadEnd() { return countActive() == 0; }

// --- RIGHT-HAND RULE ---
// Priority: right > straight > left > U-turn
int chooseTurn() {
  if (rightOpen())    return  1;
  if (straightOpen()) return  0;
  if (leftOpen())     return -1;
  return 2;
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
// LINE FOLLOWING — identical to original
// -------------------------------------------------------

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - previousLoopTime >= LOOP_INTERVAL) {
    previousLoopTime = currentTime;

    readSensors();
    debugPrint();

    // --- STATE: FOLLOW ---
    if (state == FOLLOW) {
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
    }

    // --- STATE: CROSSING ---
    else if (state == CROSSING) {
      forward();
      if (currentTime - stateTimer >= CROSSING_TIME) {
        state      = TURNING;
        stateTimer = currentTime;
      }
    }

    // --- STATE: TURNING ---
    else if (state == TURNING) {
      unsigned long needed = (pendingTurn == 2) ? TURN_180_TIME : TURN_90_TIME;
      if (pendingTurn == -1) {
        rotateMotorsLeft();
        lastDirection = -1;
      } else {
        rotateMotorsRight();
        lastDirection = 1;
      }
      if (currentTime - stateTimer >= needed) {
        state = SEARCH;
      }
    }

    // --- STATE: SEARCH ---
    else if (state == SEARCH) {
      if (pendingTurn == -1) {
        rotateMotorsLeft();
      } else {
        rotateMotorsRight();
      }
      if (anyActive(2, 5)) {
        state = FOLLOW;
      }
    }

    // --- STATE: RECOVER ---
    else if (state == RECOVER) {
      if (lastDirection == 1) {
        rotateMotorsRight();
      } else if (lastDirection == -1) {
        rotateMotorsLeft();
      } else {
        stopMotors();
      }
      if (!deadEnd()) {
        state = FOLLOW;
      }
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
  Serial.begin(9600);
  bt.begin(9600);
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
  analogWrite(MOTOR_A_1, 0);          analogWrite(MOTOR_A_2, leftSpeed);
  analogWrite(MOTOR_B_1, rightSpeed); analogWrite(MOTOR_B_2, 0);
}
void turnLeftGentle() {
  // Slow inner wheel by 120 to handle tight curves at high speed
  analogWrite(MOTOR_A_1, 0);                analogWrite(MOTOR_A_2, leftSpeed - 120);
  analogWrite(MOTOR_B_1, rightSpeed);       analogWrite(MOTOR_B_2, 0);
}
void turnRightGentle() {
  // Slow inner wheel by 120 to handle tight curves at high speed
  analogWrite(MOTOR_A_1, 0);                analogWrite(MOTOR_A_2, leftSpeed);
  analogWrite(MOTOR_B_1, rightSpeed - 120); analogWrite(MOTOR_B_2, 0);
}
void turnLeftMedium() {
  // Brake inner wheel completely for sharp corners
  analogWrite(MOTOR_A_1, 0); analogWrite(MOTOR_A_2, 0);
  analogWrite(MOTOR_B_1, rightSpeed); analogWrite(MOTOR_B_2, 0);
}
void turnRightMedium() {
  // Brake inner wheel completely for sharp corners
  analogWrite(MOTOR_A_1, 0);          analogWrite(MOTOR_A_2, leftSpeed);
  analogWrite(MOTOR_B_1, 0);          analogWrite(MOTOR_B_2, 0);
}
void rotateMotorsLeft() {
  // Emergency pivot — reverse left wheel, drive right wheel
  analogWrite(MOTOR_A_1, leftSpeed);  analogWrite(MOTOR_A_2, 0);
  analogWrite(MOTOR_B_1, rightSpeed); analogWrite(MOTOR_B_2, 0);
}
void rotateMotorsRight() {
  // Emergency pivot — drive left wheel, reverse right wheel
  analogWrite(MOTOR_A_1, 0);          analogWrite(MOTOR_A_2, leftSpeed);
  analogWrite(MOTOR_B_1, 0);          analogWrite(MOTOR_B_2, rightSpeed);
}
void stopMotors() {
  analogWrite(MOTOR_A_1, 0); analogWrite(MOTOR_A_2, 0);
  analogWrite(MOTOR_B_1, 0); analogWrite(MOTOR_B_2, 0);
}
