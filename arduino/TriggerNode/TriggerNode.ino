/** 
 * Trigger node to remotely control weather node to do something (i.e. by setting analog pin high).
 * 
 * As soon as arduino starts then it contantly sends a command (ON, ON_16S, OFF) - it is that dead simple.
 * If ACK is received, then for:
 * - ON or OFF it stops and continuesly deep sleeps 
 * - ON_16S it stops for 7.5s (mostly in deep sleep) and re-sends the command,
 *   the target node will automatically turn off analog pin when no more commands are heard.
 *   Target node awakes every 8s to listen, hence 7.5s above.
 *
 * If ACK is not received or ACK is disabled either on trigger node or target node, then it continuesly sends the command.
 * 
 * Of course code can be extended to work with tact switches/buttons or do sth programatically.
 * 
 * Author: Piotr Skowronek, piotr@skowro.net
 * License: Apache License, version 2.0
 * 
 * Based on:
 * RFM69HCW Example Sketch - https://learn.sparkfun.com/tutorials/rfm69hcw-hookup-guide/running-the-example-code
 */

// Messages (operations) the trigger can send and target unit understand
#define TURN_ON_16S   "ON_16S"    // Turn ON for 16s (2x 8s deep sleeps)
#define TURN_ON       "ON"        // Turn ON forever
#define TURN_OFF      "OFF"       // Turn OFF

// To configure the trigger node you have to edit (or duplicate) node.h and include it here:
#include "node.h"

#include <RFM69.h>
#include <LowPower.h>

// RFM69 radio
RFM69 radio;

// Whether to turn off the switch (after 2 full deep sleeps i.e. 16s)
byte sleepForever = false;

void setup() {
  Serial.begin(9600);
  Serial.print(F("Trigger node: "));
  Serial.print(THIS_NODE_ID);
  Serial.print(F(" @ network: "));
  Serial.print(NETWORK_ID);
  Serial.println(F(" is ready"));

  pinMode(PIN_LED, OUTPUT);
  initRadio();
}

void loop() {
  if (!sleepForever) {
    if (sendCommand()) {
      Serial.println(F("Going to power down for ~7s..."));
      Serial.flush();
      radio.sleep();
      // sleep for 7.5s (every 8s target node awakes for ~100ms to listen)
      analogWrite(PIN_LED, 0);
      LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); // consider narcoleptic https://github.com/brabl2/narcoleptic
      analogWrite(PIN_LED, 255);
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
      analogWrite(PIN_LED, 0);
      LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
      LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
      if (RC_RECAL) {
        radio.rcCalibration();
      }
      radio.receiveDone(); // awake radio
      Serial.println(F("Awaken!"));
      Serial.flush();
    } else {
      if (!BURST_MODE) {
        radio.sleep();
        Serial.flush();
        LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
        radio.receiveDone(); // awake radio
      }
    }
  } else {
    analogWrite(PIN_LED, 0);
    Serial.println(F("Sleeping forever, dreaming about stars..."));
    Serial.flush();
    radio.sleep();
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }  
}

bool sendCommand() {
  analogWrite(PIN_LED, 255);
  Serial.print(F("Going to send the following command: "));
  Serial.print(CMD_TO_SEND);
  Serial.print(" to node: ");
  Serial.println(TARGET_ID);
  bool ack = false;  

  if (USE_ACK) {
    if (radio.sendWithRetry(TARGET_ID, CMD_TO_SEND, strlen(CMD_TO_SEND), ACK_RETRIES, ACK_WAIT)) {
      Serial.println(F("ACK received! Good."));
      ack = true;
    } else {
      Serial.println(F("no ACK received :/")); 
    }
  } else {
    // w/o ACK
    for (int i = 0; i < ACK_RETRIES; i++) {
      radio.send(TARGET_ID, CMD_TO_SEND, strlen(CMD_TO_SEND));
      delay(5);
    }
    Serial.println(F("Data sent w/o requesting ACK"));
  }

  if (ack && strcmp(CMD_TO_SEND, TURN_ON_16S)) {  // ack + != ON_16S
    sleepForever = true;
  }
  analogWrite(PIN_LED, 0);
  return ack;
}

void initRadio() {
  radio.initialize(FREQUENCY, THIS_NODE_ID, NETWORK_ID);
  radio.setHighPower(HIGH_POWER_MODE);
  if (ENCRYPT) {
    radio.encrypt(ENCRYPT_KEY);
  }
}
