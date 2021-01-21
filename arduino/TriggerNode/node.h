/**
 * A trigger node configuration. 
 * If you have multiple nodes, then copy this file to node_XX.h and adjust configuration accordingly.
 * In the main TriggerNode.ino just change the name of a node header file.
 */
 
#define NETWORK_ID    1
#define THIS_NODE_ID  10
// Which node to control
#define TARGET_ID     2

// RFM69 frequency
#define FREQUENCY     RF69_433MHZ // or RF69_915MHZ
// RFM69 mode - rfm69hw is high-power (and you can lower the power output by setting it to false), rfm69cw is NOT high-power and you have to set it to false!
#define HIGH_POWER_MODE true
// RFM69 force RC recalibration before transmission (when transmitter is outside and freezing temps are expected)
// in other words: when amb. temperature difference between transmitter and receiver is substantial (more than >20'C)
#define RC_RECAL      true
// Send command continuously (in burst) until ACK received or do it in 60ms intervals. When operating off the battery you should set it to false.
// Mind that this may disrupt communication of other devices transmitting on the same freq.
#define BURST_MODE    false

// AES encryption (or not):
#define ENCRYPT       true // Set to "true" to use encryption
// Encryption key - make it exactly 16 bytes (add padding or whatever if necessary) to ensure RPi side can decipher (lib on RPi must have exact 16-byte key)
                     //1234567890123456
#define ENCRYPT_KEY   "yourpasswdhere.."

// whether to use ACK
#define USE_ACK       true

// how many retries until ACK received?
#define ACK_RETRIES   3
// max wait time for ACK (in ms)
#define ACK_WAIT      20

// LED PIN
#define PIN_LED       A1

// Operation/command to send
#define CMD_TO_SEND   TURN_ON_16S
