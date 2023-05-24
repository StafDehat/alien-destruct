/**
  Author: Andrew Howard
**/


// Runs on Mega

#include <Keypad.h>
const byte ROWS = 4;
const byte COLS = 3;
byte rowPins[ROWS] = {14, 15, 16, 17};
byte colPins[COLS] = {18, 19, 20};
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(7, 8); // CE, CSN
const uint8_t address[6] = "00001";

//const char passcode[8] = {'8', '6', '7', '5', '3', '0', '9', '\0'};
const char* passcode = "8675309";
const int buttonPin = 9;
const int piPin = 5;
const int laPin = 3;

int passcodeIndex;


void setup() {
  Serial.begin(9600);
  Serial.println("Beginning setup");

  passcodeIndex = 0;

  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(piPin, OUTPUT);
  pinMode(laPin, OUTPUT);
  digitalWrite(piPin, LOW);
  digitalWrite(laPin, LOW);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);

  Serial.println("Setup complete");
}


// Monitor the button.  When it's pressed, return;
void waitForArmSignalButton() {
  Serial.println("In waitForArmSignalButton()");
  while (digitalRead(buttonPin) == HIGH) {
    delay(50);
  }
}

// Monitor incoming radio signal.
void waitForArmSignalRadio() {
  Serial.println("In waitForArmSignalRadio()");
  radio.startListening();
  while (true) {
    if (! radio.available() ) {
      Serial.println("Radio was unavailable - retrying infinitely");
      delay(50);
      continue;
    }
    Serial.println("Radio available");
    radio.stopListening();
    return;
  }
}

// Arm the self-destruct sequence.
void arm() {
  Serial.println("In arm()");
  digitalWrite(piPin, HIGH);       // Toggle IoT-Relay.
  digitalWrite(laPin, HIGH);       // Open acrylic door
}

// Monitor the keypad for correct disarm code entry
void monitorKeypad() {
  Serial.println("In monitorKeypad()");
  char key;
  while ( true ) {
    delay(50);
    key = keypad.getKey();
    if (key == NO_KEY) {
      continue;
    }

    // A key's being pressed - if it weren't we could have restarted the loop
    Serial.print(key);

    // Wrong key was pressed
    if (key != passcode[passcodeIndex]) {
      passcodeIndex = 0;
      // But... if that wrong key was also '8', then they're 1 step into a fresh attempt
      if (key == passcode[0]) {
        passcodeIndex++;
      }
      continue;
    }

    // If they'd hit the wrong key, we would have restarted our loop.
    // Being here means they must have hit the correct key.

    // Correct key, but not finished yet:
    // 8675309 '\0'
    // 01234567
    if (passcodeIndex < 6) {
      passcodeIndex++;
      continue;
    }

    // Just correctly punched the 7th (pos=6) key.  ie: Finished
    passcodeIndex = 0;
    break;
  } //End-while
  return;
}

// Stop the self-destruct sequence.
void disarm() {
  Serial.println("In disarm()");
  digitalWrite(piPin, LOW);
  digitalWrite(laPin, LOW);
  // Send notification to Jared?
}


void loop() {
//  waitForArmSignalButton();
  waitForArmSignalRadio();
  arm();
  monitorKeypad();
  disarm();
}

