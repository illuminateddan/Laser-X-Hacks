
/*
 * LASER X Land Mine Code
 * Designed for use with the Laser X "laser" (infrared) shooting game.
 * This code requires a PIR, an IR receiver, an IR LED (or multiple with a transistor), an RGB led,
 * and probably batteries and a project box etc, etc
 * 
 * Game Play: 
 * A player shoots the circuit, arming the mine for their team. A brief countdown begins and then the circuit fires
 * multiple "shots" (the teams IR code - The unit fires back the activating IR signal). 
 * Red team kills blue, 
 * blue team kills red
 * Renegade kills everyone including itself
 */

#define redPin 11
#define greenPin 10
#define bluePin 9
#define PIRPin 12
#define ledPin 13

#include <IRremote.h>
int RECV_PIN = 2;
IRsend irsend;////An IR LED must be connected to Arduino PWM pin 3.
IRrecv irrecv(RECV_PIN);
decode_results results;

int codeType = -1; // The type of code
unsigned int rawCodes[RAWBUF]; // The durations if raw
int codeLen; // The length of the code

// IR codes for Laser X units
unsigned int teamA[] =  {6050, 650, 400, 600, 1450, 600, 450, 600, 1450, 600, 450, 600, 400, 600, 1400, 650, 450};
unsigned int teamB[] =  {6150, 550, 450, 600, 1450, 600, 400, 650, 1450, 550, 450, 600, 450, 550, 450, 600, 1400};
unsigned int renegade[] = {6100, 600, 450, 550, 1450, 600, 450, 600, 1450, 600, 450, 550, 400, 650, 1450, 600, 1500};

// 33EBDD7B - 871095675 A
// A3F34992 - 2750630290 B
// 33EBDD7A - 871095674 Renegade

int attackState = 0; // (0 everyone, 1 reds own, 2 blues own)
int attackTimes = 5;
bool active = 0;

long timer = 0;
int duration = 10000;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello! Laser X Mine!");
  irrecv.enableIRIn(); // Start the receiver

  pinMode(ledPin, OUTPUT);
  pinMode(PIRPin, INPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  delay(3000);
  digitalWrite(3, LOW);

  digitalWrite(ledPin, LOW);
  setLEDFlash(255, 0, 0, 1, 100);
  setLEDFlash(0, 255, 0, 1, 100);
  setLEDFlash(0, 0, 255, 1, 100);


} // setup ------------------------------------------------------------





void loop() {

  bool PIRState = digitalRead(PIRPin);
  digitalWrite(ledPin, PIRState);

  Serial.print("Active:"); Serial.print(active);
  Serial.print(" attackState:"); Serial.print(attackState);
  Serial.print(" PIR:"); Serial.println(PIRState);

  if (active) {
    Serial.print("Active so...");
    if (PIRState) {
      Serial.print("Movement Detected: Attack ");
      switch (attackState) {
        case 0:
          sendRenegade(attackTimes);
          active = 0;
          attackState = 0;
          irrecv.enableIRIn(); // Re-enable receiver
          break;
        case 1:
          sendTeamA(attackTimes);
          active = 0;
          attackState = 0;
          irrecv.enableIRIn(); // Re-enable receiver
          break;
        case 2:
          sendTeamB(attackTimes);
          active = 0;
          attackState = 0;
          irrecv.enableIRIn(); // Re-enable receiver
          break;
      } // PIR state


    } // if PIR
  } // active



  if (!active) {

    if (irrecv.decode(&results)) {
      Serial.println("results in...");
      storeCode(&results);
      // irrecv.resume(); // resume receiver
      //Serial.println(results.value, HEX);
      Serial.print(results.value);
      Serial.print(" - ");
      irrecv.resume(); // resume receiver NOTE: NEEDS TIME TO DO OTHER STUFF BEFORE PROCESSING IR OR YOU GET BAD CODES

      switch (results.value) {
        case 871095675:
          Serial.println("Red Team Hit");
          setLEDFlash(255, 0, 0, 5, 500);
          setLEDFlash(255, 0, 0, 5, 200);
          setLEDFlash(255, 0, 0, 5, 100);
          setLED(255, 0, 0);
          attackState = 1;
          active = 1;
          break;
        case 2750630290:
          Serial.println("Blue Team Hit");
          setLEDFlash(0, 0, 255, 5, 500);
          setLEDFlash(0, 0, 255, 5, 200);
          setLEDFlash(0, 0, 255, 5, 100);
          setLED(0, 0, 255);
          attackState = 2;
          active = 1;
          break;
        case 871095674:
          Serial.println("Renegade Hit");
          setLEDFlash(255, 0, 255, 5, 500);
          setLEDFlash(255, 0, 255, 5, 200);
          setLEDFlash(255, 0, 255, 5, 100);
          setLED(255, 0, 255);
          attackState = 0;
          active = 1;
          break;
        default :
          Serial.println("Not recognised ");
          setLEDFlash(0, 255, 0, 10, 200);
          setLED(0, 255, 0);
          attackState = 0;
          active = 0;
          break;
      }// switch

    } // if receive
  } //not active

} // loop ------------------------------------------------------------




void storeCode(decode_results * results) {
  codeType = results->decode_type;
  int count = results->rawlen;
  if (codeType == UNKNOWN) {
    //Serial.println("Received unknown code, saving as raw");
    codeLen = results->rawlen - 1;
    // To store raw codes:
    // Drop first value (gap)
    // Convert from ticks to microseconds
    // Tweak marks shorter, and spaces longer to cancel out IR receiver distortion
    for (int i = 1; i <= codeLen; i++) {
      if (i % 2) {
        // Mark
        rawCodes[i - 1] = results->rawbuf[i] * USECPERTICK - MARK_EXCESS;
        //Serial.print(" m");
      } else { // i%2
        // Space
        rawCodes[i - 1] = results->rawbuf[i] * USECPERTICK + MARK_EXCESS;
        //Serial.print(" s");
      }
      //Serial.print(rawCodes[i - 1], DEC);
    }
    //Serial.println("");
  } // if codetype unknown
}

void sendCode() { // used to bounce back the IR code
  if (codeType == UNKNOWN /* i.e. raw */) {

    irsend.sendRaw(rawCodes, codeLen, 38); // Assume 38 KHz
    Serial.println("Sent raw");
  }
} // void

void sendTeamA(int _times) {
  for (int i = 0; i < _times; i++) {
    setLED(255, 255, 255);
    irsend.sendRaw(teamA, sizeof(teamA) / sizeof(teamA[0]), 38); // Assume 38 KHz
    Serial.println("Sent Team A Fire Code");
    delay(1000);
    setLED(0, 0, 0);
    delay(500);
  }
}

void sendTeamB(int _times) {
  for (int i = 0; i < _times; i++) {
    setLED(255, 255, 255);
    irsend.sendRaw(teamB, sizeof(teamB) / sizeof(teamB[0]), 38); // Assume 38 KHz
    Serial.println("Sent Team B Fire Code");
    delay(1000);
    setLED(0, 0, 0);
    delay(500);
  }
}

void sendRenegade(int _times) {
  for (int i = 0; i < _times; i++) {
    setLED(255, 255, 255);
    irsend.sendRaw(renegade, sizeof(renegade) / sizeof(renegade[0]), 38); // Assume 38 KHz
    Serial.println("Sent Renegade Fire Code");
    delay(1000);
    setLED(0, 0, 0);
    delay(500);
  }
}

void setLED(int _r, int _g, int _b) {
  analogWrite(redPin, _r);
  analogWrite(greenPin, _g);
  analogWrite(bluePin, _b);
}

void setLEDFlash(int _r, int _g, int _b, int _times, int _delay) {
  for (int i = 0; i < _times; i++) {
    analogWrite(redPin, _r);
    analogWrite(greenPin, _g);
    analogWrite(bluePin, _b);
    delay(_delay);
    analogWrite(redPin, 0);
    analogWrite(greenPin, 0);
    analogWrite(bluePin, 0);
    delay(_delay);
  }
}

