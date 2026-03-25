#include <Adafruit_NeoPixel.h>

#define LED_PIN 7
#define NUM_LEDS 4

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

const int MOTOR_A_1 = 3;
const int MOTOR_A_2 = 5;
const int MOTOR_B_1 = 6;
const int MOTOR_B_2 = 9;


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

void turnLeft() {
  pixels.setPixelColor(3, pixels.Color(100, 100, 0));    // front-left
  pixels.show();
}

void turnRight() {
  pixels.setPixelColor(2, pixels.Color(100, 100, 0));    // front-right
  pixels.show();
}

void backwardsLight() {
  pixels.setPixelColor(1, pixels.Color(100, 0, 0));    // back right
  pixels.setPixelColor(0, pixels.Color(100, 0, 0));    // back-left
  pixels.show();
}

void frontLights()  {
  pixels.setPixelColor(2, pixels.Color(0, 100, 0));    // front-right
  pixels.setPixelColor(3, pixels.Color(0, 100, 0));    // front-left
  ´pixels.show();
}

void lightsOff() {
  pixels.clear();
  pixels.show();
}

void setup() {

  pinMode(MOTOR_A_1, OUTPUT);
  pinMode(MOTOR_A_2, OUTPUT);
  pinMode(MOTOR_B_1, OUTPUT);
  pinMode(MOTOR_B_2, OUTPUT);

  pixels.begin();
  pixels.clear();

  pixels.show();

}

void loop() { 

}
