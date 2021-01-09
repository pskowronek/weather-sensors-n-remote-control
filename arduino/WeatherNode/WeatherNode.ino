/** 
 * Weather node with remote control capability.
 * 
 * Author: Piotr Skowronek, piotr@skowro.net
 * License: Apache License, version 2.0
 * 
 * Based on:
 * RFM69HCW Example Sketch - https://learn.sparkfun.com/tutorials/rfm69hcw-hookup-guide/running-the-example-code
 * BME280 Example - https://github.com/LowPowerLab/RFM69/blob/master/Examples/WeatherNode/WeatherNode.ino
 */



#include <RFM69.h>
#include <SPI.h>
#include <LowPower.h>
#include <SparkFunBME280.h>


#define NETWORK_ID    1
#define THIS_NODE_ID  2
// A short name that could be used for data visualization, use alphanumeric & us-ascii only, don't use commas!
// Please do remember that the whole packet max data size is 61 bytes and it must accomodate sensor readings.
#define THIS_NODE_NAME "hall"
#define GATEWAY_ID    1

// A packet format: nm -> name, t -> temp*10, p -> pressure (hPa), h -> humidity, v -> voltage (mV), r -> last RSSI
#define PACKET_FORMAT "nm:%s,t:%d,p:%d,h:%d,v:%d,r:%d"

// RFM69 frequency
#define FREQUENCY     RF69_433MHZ // or RF69_915MHZ

// AES encryption (or not):
#define ENCRYPT       true // Set to "true" to use encryption
// Encryption key - make it exactly 16 bytes (add padding or whatever if necessary) to ensure RPi side can decipher (lib on RPi must have exact 16-byte key)
                     //1234567890123456
#define ENCRYPT_KEY   "yourpasswdhere.."

// whether to use ACK
#define USE_ACK       true

// how many retries until ACK received?
#define ACK_RETRIES   3
// max wait time for ACK
#define ACK_WAIT      20

// BME280 conf'n
#define BME280_ADDRESS 0x76 // or 0x77
// I2C bus speed
#define BME280_I2C_CLOCK 400000

// Transmit every n 8s long deep sleep events (to send every ~80s set the value to 10)
#define SEND_EVERY_N_8S 112       // 112 -> ~15m (~8*112=~896 -> ~900s -> ~15m)

// Whether to wait for commands
#define ACCEPT_COMMANDS true      // to conserve energy you may turn it off if you don't need to switch on/off external device.

// Messages (operations) the unit can receive and handle
#define TURN_ON_16S   "ON_16S"    // Turn ON for 16s (2x 8s deep sleeps)
#define TURN_ON       "ON"        // Turn ON forever
#define TURN_OFF      "OFF"       // Turn OFF

// The analog pin to set high for turning ON, to set low when OFF. Use NPN transitor to control external device.
#define PIN_SWITCH    A1

// RFM69 radio
RFM69 radio;
// BME280 sensor
BME280 bme;

// A counter used to know when to transmit
int transmitCounter = SEND_EVERY_N_8S; // start transmission when it is powered up (ie after 8s of sleep) - to verify if it works w/o waiting too much time.
// Whether to turn off the switch (after 2 full deep sleeps i.e. 16s)
byte turnOffCountDown = 0;
// Last RSSI
int lastRSSI = 0;

void setup() {
  Serial.begin(9600);
  Serial.print(F("Weather Node: "));
  Serial.print(THIS_NODE_ID);
  Serial.print(F(" @ network: "));
  Serial.print(NETWORK_ID);
  Serial.println(F(" is ready"));

  pinMode(PIN_SWITCH, OUTPUT);

  initBme280();
  initRadio();
}

void loop() {
  if (transmitCounter > SEND_EVERY_N_8S) {
    transmitCounter = 0;
    transmitMeasurements();
  }
  Serial.println(F("Going to power down for ~8s..."));
  Serial.flush();
  radio.sleep();
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); // consider narcoleptic https://github.com/brabl2/narcoleptic
  Serial.flush();
  Serial.println(F("Awaken!"));
  Serial.flush();
  if (ACCEPT_COMMANDS) {
    if (turnOffCountDown > 0) {
      turnOffCountDown--;
    }
    handleReceives(); // wake up radio and check if something is received
    LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);  // briefly power down with radio on to let it catch something :)
    handleReceives(); // re-check if something was received
  }
  transmitCounter++;  
}

void handleReceives() {
  if (radio.receiveDone()) {
    Serial.print(F("Got message from node: "));
    Serial.print(radio.SENDERID);
    Serial.print(", message [");
    for (byte i = 0; i < radio.DATALEN; i++) {
      Serial.print((char)radio.DATA[i]);
    }
    Serial.print("], RSSI ");
    Serial.println(radio.RSSI);
    if (radio.ACKRequested()) {
      radio.sendACK();
      Serial.println("ACK sent");
    }
    if (!strcmp(radio.DATA, TURN_ON_16S)) {
      Serial.println(F("Going to turn on for ~8-16s..."));
      analogWrite(PIN_SWITCH, 255);
      turnOffCountDown = 3;
    } else if (!strcmp(radio.DATA, TURN_ON)) {
      Serial.println(F("Going to turn on forever..."));
      analogWrite(PIN_SWITCH, 255);
      turnOffCountDown = 0;
    } else if (turnOffCountDown == 1 or !strcmp(radio.DATA, TURN_OFF)) {
      turnOffCountDown = 1;
    } else {
      Serial.print(F("Unknown command: "));
      Serial.println(radio.DATA[0]);
    }
  }
  if (turnOffCountDown == 1) {
    Serial.println(F("Going to turn off..."));
    analogWrite(PIN_SWITCH, 0);
    turnOffCountDown = 0;
  }
  Serial.flush();
}

void transmitMeasurements() {
  char buffer[61];
  fillMeasurements(&buffer[0]);
  Serial.println(F("Going to send data!"));
  if (USE_ACK) {
    if (radio.sendWithRetry(GATEWAY_ID, buffer, strlen(buffer), ACK_RETRIES, ACK_WAIT)) {
      Serial.println(F("ACK received! Good."));
    } else {
      Serial.println(F("no ACK received :/")); 
    }
  } else {
    radio.send(GATEWAY_ID, buffer, strlen(buffer));
    Serial.println(F("Data sent w/o requesting ACK"));
  }
  lastRSSI = radio.readRSSI(false);
  Serial.flush();
}

void fillMeasurements(char* buffer) {
  bme.setMode(MODE_FORCED);
  sprintf(buffer, PACKET_FORMAT,
      THIS_NODE_NAME,
      (int)(bme.readTempC()*10),
      (int)(bme.readFloatPressure() / 100.0F),
      (int)bme.readFloatHumidity(),
      (int)readVcc(),
      (int)lastRSSI); 
  bme.setMode(MODE_SLEEP);
  Serial.print(F("Got the following measurements: "));
  Serial.println(buffer);
  Serial.flush();
}

void initRadio() {
  radio.initialize(FREQUENCY, THIS_NODE_ID, NETWORK_ID);
  radio.setHighPower(true);
  if (ENCRYPT) {
    radio.encrypt(ENCRYPT_KEY);
  }
}

void initBme280() {
  // based on https://github.com/LowPowerLab/RFM69/blob/master/Examples/WeatherNode/WeatherNode.ino
  Wire.begin();
  Wire.setClock(BME280_I2C_CLOCK);
  bme.setI2CAddress(BME280_ADDRESS);
  bme.beginI2C();
  bme.setMode(MODE_FORCED); //MODE_SLEEP, MODE_FORCED, MODE_NORMAL is valid. See 3.3
  bme.setStandbyTime(0); //0 to 7 valid. Time between readings. See table 27.
  bme.setFilter(0); //0 to 4 is valid. Filter coefficient. See 3.4.4
  bme.setTempOverSample(1); //0 to 16 are valid. 0 disables temp sensing. See table 24.
  bme.setPressureOverSample(1); //0 to 16 are valid. 0 disables pressure sensing. See table 23.
  bme.setHumidityOverSample(1);
  bme.setMode(MODE_SLEEP);
}

// Read input voltage (battery status),
// taken from: https://www.instructables.com/Secret-Arduino-Voltmeter/ + https://code.google.com/archive/p/tinkerit/wikis/SecretVoltmeter.wiki
long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
     ADMUX = _BV(MUX5) | _BV(MUX0) ;
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
 
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
 
  long result = (high<<8) | low;
 
  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}
