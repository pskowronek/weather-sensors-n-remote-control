/** 
 * Weather node with optional remote control capability.
 * 
 * The node should have the following modules connected:
 * - RFM69 for connectivity
 * - BME280 for temperature & humidity
 * - TSL2561 for luminosity (optionally, if USE_LUMI_SENSOR defined below).
 *   Mind that this increases power consumption (adds ~100ms of ~0.5mA consumption during periodical measurements)
 * 
 * See more details under: https://github.com/pskowronek/weather-sensors-n-remote-control
 * 
 * Author: Piotr Skowronek, piotr@skowro.net
 * License: Apache License, version 2.0
 * 
 * Based on:
 * RFM69HCW Example Sketch - https://learn.sparkfun.com/tutorials/rfm69hcw-hookup-guide/running-the-example-code
 * BME280 Example - https://github.com/LowPowerLab/RFM69/blob/master/Examples/WeatherNode/WeatherNode.ino
 */

// To configure the node you have to edit (or duplicate) node.h and include it here:
#include "node.h"

#include <RFM69.h>
#include <SPI.h>
#include <LowPower.h>
#include <SparkFunBME280.h>

#ifdef WATCHDOG
  #include <avr/wdt.h>
#endif

// whether TSL2561 lumi sensor is attached
#ifdef USE_LUMI_SENSOR
  #include <Adafruit_Sensor.h>
  #include <Adafruit_TSL2561_U.h>
#endif


// A packet format: nm -> name, t -> temp*10, p -> pressure (hPa), h -> humidity, l -> lux, v -> voltage (mV), r -> last RSSI, u -> uptime (in days)
#define PACKET_FORMAT "nm:%s,t:%d,p:%d,h:%d,l:%d,v:%d,r:%d,u:%d"

// RFM69 radio
RFM69 radio;
// BME280 sensor
BME280 bme;

// TSL2561 lumi sensor
#ifdef USE_LUMI_SENSOR
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
#endif

// A counter used to know when to transmit
int transmitCounter = SEND_EVERY_N_8S; // start transmission when it is powered up (ie after 8s of sleep) - to verify if it works w/o waiting too much time.
// Whether to turn off the switch (after 2 full deep sleeps i.e. 16s)
byte turnOffCountDown = 0;
// Last RSSI
int lastRSSI = 0;
// to more-or-less keep time while sleeping (for uptime) - in ms
unsigned long sleepTimeCounter = 0;

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
  #ifdef USE_LUMI_SENSOR
  initLumi();
  #endif
}

void loop() {
#ifdef WATCHDOG
  wdt_enable(WDTO_4S);
#endif
  if (ACCEPT_COMMANDS) {
    if (turnOffCountDown > 0) {
      turnOffCountDown--;
    }
    if (RC_RECAL) {
      radio.rcCalibration();
    }
    handleReceives(); // wake up radio and check if something is received
#ifdef WATCHDOG
    wdt_reset();
    wdt_disable();
#endif
    LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);  // briefly power down with radio on to let it catch something :)
#ifdef WATCHDOG
    wdt_enable(WDTO_4S);
#endif
    sleepTimeCounter += 120;
    handleReceives(); // re-check if something was received
  }
  if (transmitCounter > SEND_EVERY_N_8S) {
    transmitCounter = 0;
    if (RC_RECAL) {
      radio.rcCalibration();
    }
    transmitMeasurements();
  }
  Serial.println(F("Going to power down for ~8s..."));
  Serial.flush();
  radio.sleep();
#ifdef WATCHDOG
  wdt_reset();
  wdt_disable();
#endif
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); // consider narcoleptic https://github.com/brabl2/narcoleptic (and avoid sleepTimeCounter)
  sleepTimeCounter += 8*1000;
  Serial.flush();
  Serial.println(F("Awaken!"));
  Serial.flush();

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
  if (TURN_OFF_WHILE_SENDING && turnOffCountDown != 0) {
      analogWrite(PIN_SWITCH, 0);
  }
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
  if (TURN_OFF_WHILE_SENDING && turnOffCountDown != 0) {
      analogWrite(PIN_SWITCH, 255);
  }
  lastRSSI = radio.readRSSI(false);
  Serial.flush();
}

void fillMeasurements(char* buffer) {
  int lumi = -1;
  #ifdef USE_LUMI_SENSOR
    sensors_event_t event;
    tsl.getEvent(&event);
    lumi = event.light;
  #endif
  int uptime = (millis() + sleepTimeCounter) / (1000L*3600*24);
  bme.setMode(MODE_FORCED);
  sprintf(buffer, PACKET_FORMAT,
      THIS_NODE_NAME,
      (int)(bme.readTempC()*10),
      (int)(bme.readFloatPressure() / 100.0F),
      (int)bme.readFloatHumidity(),
      lumi,
      (int)readVcc(),
      lastRSSI,
      uptime); 
  bme.setMode(MODE_SLEEP);
  Serial.print(F("Got the following measurements: "));
  Serial.println(buffer);
  Serial.flush();
}

void initRadio() {
  radio.initialize(FREQUENCY, THIS_NODE_ID, NETWORK_ID);
  radio.setHighPower(HIGH_POWER);
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

#ifdef USE_LUMI_SENSOR
void initLumi() {
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  if (!tsl.begin()) {   // module is automatically put into low-power mode
    Serial.print(F("Lumi sensor not found! Going to continue nevertheless..."));
  }
}
#endif

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
