const int SERVO_PIN = 9;
const int OPEN_US  = 1900;
const int CLOSE_US = 1100;

int servoTargetUs = OPEN_US;
long lastServoFrameMs = 0;

void servoUpdate() {
  long now = millis();
  if (now - lastServoFrameMs >= 20) {
    lastServoFrameMs = now;
    digitalWrite(SERVO_PIN, HIGH);
    delayMicroseconds(servoTargetUs);
    digitalWrite(SERVO_PIN, LOW);
  }
}
void runFor(long ms) {
  long start = millis();
  while (millis() - start < ms) {
    servoUpdate(); // keep servo active
  }
}

const int AIN1 = 7;
const int AIN2 = 8;
const int BIN1 = 10;
const int BIN2 = 11;

void motorsStop() {
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
}

void motorsForward(int spd) {
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
}

void setup() {
  pinMode(SERVO_PIN, OUTPUT);

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

  // Step 1: open gripper
  servoTargetUs = OPEN_US;
  runFor(800);

  // Step 2: drive forward 2s
  motorsForward(180);
  runFor(2000);
  motorsStop();

  // Step 3: close gripper 1s
  servoTargetUs = CLOSE_US;
  runFor(1000);

  // Step 4: drive forward 2s holding cone
  motorsForward(180);
  runFor(2000);
  motorsStop();

  // Step 1: open gripper
  servoTargetUs = OPEN_US;
  runFor(800);
}

void loop() {
  servoUpdate(); // keep holding
}
