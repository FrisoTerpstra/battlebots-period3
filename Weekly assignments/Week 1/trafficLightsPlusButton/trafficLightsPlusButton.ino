const int buttonPin = 7;
const int redPin = 8;
const int yellowPin = 12;
const int greenPin = 13;

void setup() {
  pinMode(redPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  // Initial State: Red is ON, others OFF
  // Using reverse logic (LOW = ON)
  digitalWrite(redPin, LOW);
  digitalWrite(yellowPin, HIGH);
  digitalWrite(greenPin, HIGH);
}

void loop() {
  // Check if button is pressed
  if (digitalRead(buttonPin) == LOW) {
    delay(1000); // Wait 1 second before sequence starts

    // Switch from Red to Green
    digitalWrite(redPin, HIGH);
    digitalWrite(greenPin, LOW);
    delay(3000); // Stay green for 3 seconds

    // Switch from Green to Yellow
    digitalWrite(greenPin, HIGH);
    digitalWrite(yellowPin, LOW);
    delay(1000); // Stay yellow for 1 second

    // Switch from Yellow back to Red
    digitalWrite(yellowPin, HIGH);
    digitalWrite(redPin, LOW);
    
    delay(500); // Debounce delay to prevent repeat triggers
  }
}
