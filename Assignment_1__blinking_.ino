const int buttonPin1 = 7;
const int buttonPin2 = 2;
const int buttonPin3 = 4;
const int ledPin = 8;

int mode = 0;
int lastB1 = HIGH;
int lastB2 = HIGH;
int lastB3 = HIGH;


// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);
}

// the loop function runs over and over again forever
void loop() {
  int b1 = digitalRead(buttonPin1);
  int b2 = digitalRead(buttonPin2);
  int b3 = digitalRead(buttonPin3);

    if (lastB1 == HIGH && b1 == LOW) {
    mode = 1;               // fast mode
  }

  // Detect a PRESS (HIGH -> LOW) on button 2
  if (lastB2 == HIGH && b2 == LOW) {
    mode = 2;               // slow mode
  }

   // Detect a PRESS (HIGH -> LOW) on button 3
  if (lastB3 == HIGH && b3 == LOW) {
    mode = 3;               // normal mode
  }

  lastB1 = b1;
  lastB2 = b2;
  lastB3 = b3;

  // Blink based on the selected mode
  if (mode == 1) {          // fast: twice per second
    digitalWrite(ledPin, HIGH); delay(250);
    digitalWrite(ledPin, LOW);  delay(250);
  } else if (mode == 2) {   // slow: once every 2 seconds
    digitalWrite(ledPin, HIGH); delay(2000);
    digitalWrite(ledPin, LOW);  delay(2000);
  } else if (mode == 2) {   // normal: once per second
    digitalWrite(ledPin, HIGH); delay(1000);
    digitalWrite(ledPin, LOW);  delay(1000);
  } else {                  // normal: once per second
    digitalWrite(ledPin, HIGH); delay(1000);
    digitalWrite(ledPin, LOW);  delay(1000);
  }
}
