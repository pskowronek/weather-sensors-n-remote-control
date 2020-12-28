from RFM69 import Radio, FREQ_433MHZ
import datetime
import time

node_id = 1
network_id = 1
#                 1234567890123456 - 16 chars/bytes
encryption_key = 'skowropaddddding'


print("Starting...")
with Radio(FREQ_433MHZ, node_id, network_id, isHighPower=True, encryptionKey=encryption_key, verbose=True, auto_acknowledge=True) as radio:
    #print (radio.read_temperature())
    radio.calibrate_radio()
    radio.begin_receive()

    print ("Starting loop...")

    while True:
        time.sleep(1)
        print("Sending....")
        res = radio.send(1, "TEST", attempts=3, wait=200, require_ack=True)
        print(res)

        #print ("Test")
        # Process packets
        for packet in radio.get_packets():
            print (packet)

