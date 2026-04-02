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

// --- DIRECTION MEMORY (for lost-line recovery) ---
// -1 = last turned left, 1 = last turned right, 0 = never moved
int lastDirection = 0;

unsigned long previousLoopTime = 0;
const long LOOP_INTERVAL = 10;

void setup() {
  pinMode(MOTOR_A_1, OUTPUT);
  pinMode(MOTOR_A_2, OUTPUT);
  pinMode(MOTOR_B_1, OUTPUT);
  pinMode(MOTOR_B_2, OUTPUT);

  for (int i = 0; i < 8; i++) {
    pinMode(SENSOR_PINS[i], INPUT);
  }

  Serial.begin(9600);
}


void loop() {
  unsigned long currentTime = millis();

  if (currentTime - previousLoopTime >= LOOP_INTERVAL) {
    previousLoopTime = currentTime;

    for (int i = 0; i < 8; i++) {
      val[i] = analogRead(SENSOR_PINS[i]);
    }

    // --- CENTER: D4 or D5 ---
    // Line is centred — go max speed
    if (val[3] > threshold || val[4] > threshold) {
      forward();
    }

    // --- SLIGHT DRIFT: D3 or D6 ---
    else if (val[2] > threshold) {
      lastDirection = 1;
      turnRightGentle();
    }
    else if (val[5] > threshold) {
      lastDirection = -1;
      turnLeftGentle();
    }

    // --- MEDIUM DRIFT: D2 or D7 ---
    else if (val[1] > threshold) {
      lastDirection = 1;
      turnRightMedium();
    }
    else if (val[6] > threshold) {
      lastDirection = -1;
      turnLeftMedium();
    }

    // --- EXTREME DRIFT: D1 or D8 ---
    else if (val[0] > threshold) {
      lastDirection = 1;
      rotateMotorsRight();
    }
    else if (val[7] > threshold) {
      lastDirection = -1;
      rotateMotorsLeft();
    }

    // --- LOST LINE ---
    // Keep rotating in the last known direction to sweep back onto the line
    else {
      if (lastDirection == 1) {
        rotateMotorsRight();
      } else if (lastDirection == -1) {
        rotateMotorsLeft();
      } else {
        stopMotors(); // Only stops if the line was never detected
      }
    }
  }
}

void forward() {
  analogWrite(MOTOR_A_1, 0);          analogWrite(MOTOR_A_2, leftSpeed);
  analogWrite(MOTOR_B_1, rightSpeed); analogWrite(MOTOR_B_2, 0);
}

void turnLeftGentle() {
  // Slow inner wheel by 120 to handle tight curves at high speed
  analogWrite(MOTOR_A_1, 0);                      analogWrite(MOTOR_A_2, leftSpeed - 120);
  analogWrite(MOTOR_B_1, rightSpeed);             analogWrite(MOTOR_B_2, 0);
}

void turnRightGentle() {
  // Slow inner wheel by 120 to handle tight curves at high speed
  analogWrite(MOTOR_A_1, 0);          analogWrite(MOTOR_A_2, leftSpeed);
  analogWrite(MOTOR_B_1, rightSpeed - 120);       analogWrite(MOTOR_B_2, 0);
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
