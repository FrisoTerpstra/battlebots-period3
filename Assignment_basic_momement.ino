const int MOTOR_A_1 = 3;
const int MOTOR_A_2 = 11;
const int MOTOR_B_1 = 9;
const int MOTOR_B_2 = 10;

void forward() { // go foreward
  digitalWrite(MOTOR_A_1, LOW); digitalWrite(MOTOR_A_2, HIGH);
  digitalWrite(MOTOR_B_1, HIGH); digitalWrite(MOTOR_B_2, LOW);
}

void backwards() { // go backward
  digitalWrite(MOTOR_A_1, HIGH); digitalWrite(MOTOR_A_2, LOW);
  digitalWrite(MOTOR_B_1, LOW); digitalWrite(MOTOR_B_2, HIGH);
}

void rotateMotorsLeft() { // turn left
  digitalWrite(MOTOR_A_1, HIGH); digitalWrite(MOTOR_A_2, LOW);
  digitalWrite(MOTOR_B_1, HIGH); digitalWrite(MOTOR_B_2, LOW);
}

void rotateMotorsRight() { // turn right
  digitalWrite(MOTOR_A_1, LOW); digitalWrite(MOTOR_A_2, HIGH);
  digitalWrite(MOTOR_B_1, LOW); digitalWrite(MOTOR_B_2, HIGH);
}

void stopMotors() { // stop the motors
  digitalWrite(MOTOR_A_1, LOW); digitalWrite(MOTOR_A_2, LOW);
  digitalWrite(MOTOR_B_1, LOW); digitalWrite(MOTOR_B_2, LOW); 
}

void setup() {
  pinMode(MOTOR_A_1, OUTPUT);
  pinMode(MOTOR_A_2, OUTPUT);
  pinMode(MOTOR_B_1, OUTPUT);
  pinMode(MOTOR_B_2, OUTPUT);

  // do the assignment Basic Moves  
  forward();
  delay(4000);
  stopMotors();
  delay(1000);
  backwards();
  delay(4000);
  stopMotors();
  delay(1000);
  rotateMotorsLeft();
  delay(775);
  stopMotors();
  delay(1000);
  rotateMotorsRight();
  delay(775);
  stopMotors();
  
}

void loop() {

}
