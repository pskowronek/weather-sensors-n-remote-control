/**
 * A weather node configuration. 
 * If you have multiple nodes, then copy this file to node_XX.h and adjust configuration accordingly.
 * In the main WeatherNode.ino just change the name of a node header file.
 */

// whether TSL2561 lumi sensor is connected
//#define USE_LUMI_SENSOR

// whether to unleash built-in watchdog to reboot when it appears the device is not responding (transmitting)
// BE WARNED - on Arduino Pro Mini it didn't work for me (boot loop) - https://github.com/arduino/ArduinoCore-avr/issues/150
//#define WATCHDOG


#define NETWORK_ID    1
#define THIS_NODE_ID  2
// A short name (max 8 chars if Lumi is on) that could be used for data visualization, use alphanumeric & us-ascii only, don't use commas!
// Please do remember that the whole packet max data size is 61 bytes and it must accomodate sensor readings.
#define THIS_NODE_NAME "hall"
#define GATEWAY_ID    1

// RFM69 frequency
#define FREQUENCY     RF69_433MHZ // or RF69_915MHZ
// RFM69 mode - rfm69hw is high-power (and you can lower the power output by setting it to false), rfm69cw is NOT high-power and you have to set it to false!
#define HIGH_POWER    true
// RFM69 force RC recalibration before transmission (when transmitter is outside and freezing temps are expected)
// in other words: when amb. temperature difference between transmitter and receiver is substantial (more than >20'C)
#define RC_RECAL      true

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

// Turn off PIN_SWITCH while sending measurements (if controlled device is powered from the same power source it
// may help to stay within max current limits)
#define TURN_OFF_WHILE_SENDING true
