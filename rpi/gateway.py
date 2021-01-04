from RFM69 import Radio, FREQ_433MHZ
from influxdb import InfluxDBClient
import atexit

import datetime
import os
import sys
import logging
import logging.handlers
import sdnotify
import signal
import time


# The network and node ids
network_id = 1
node_id = 1
# And encryption key - it must be exactly 16 chars long
#                 1234567890123456 - 16 chars/bytes
encryption_key = 'yourpasswdhere..'

shutting_down = False


def main():
    global network_id
    global node_id
    global encryption_key

    atexit.register(shutdown_hook)
    signal.signal(signal.SIGTERM, signal_hook)

    # Init db connection
    logging.info("Going to connect to InfluxDB...")
    client = InfluxDBClient(host='localhost', port=8086, username='user', password='changeme', ssl=False, verify_ssl=False)
    logging.info(client.get_list_database())
    logging.info("Connected")

    client.switch_database('sensors')

    # Listening...
    logging.info("Going to start...")
    with Radio(FREQ_433MHZ, node_id, network_id, isHighPower=True, encryptionKey=encryption_key, verbose=True, auto_acknowledge=True) as radio:
        radio.calibrate_radio()
        radio.begin_receive()

        logging.info("...listening...")

        notifier = sdnotify.SystemdNotifier()
        notifier.notify("READY=1")

        while True:
            notifier.notify("WATCHDOG=1")
            time.sleep(1)
            # Process packets
            for packet in radio.get_packets():
                logging.info("Got packet!")
                logging.debug(packet)
                data = bytearray(packet.data).decode()
                # TODO better parsing etc
                data_parts = data.split(',')
                data_network_id = int(data_parts[0])
                data_node_id = int(data_parts[1])
                data_temp = int(data_parts[2]) / 10   # temp was *10 to avoid doing float -> str on arduino side
                data_press = int(data_parts[3])
                data_humi = int(data_parts[4])
                data_voltage = int(data_parts[5]) / 1000  # mV to V
                data_rssi = int(data_parts[6])      # last known RSSI, most often zero - to be fixed on Arduino side
                if data_node_id != packet.sender:
                    logging.info("Got mismatched packet - node configured as {} but sent from {}!".format(data_node_id, packet.sender))
                points = [{
                    'measurement': 'readings',
                    'time': datetime.datetime.utcnow(),
                    'tags': {
                        'network': data_network_id,
                        'node': data_node_id
                    },
                    'fields': {
                        'temp': data_temp,
                        'pressure': data_press,
                        'humidity': data_humi,
                        'voltage': data_voltage,
                        'rssi-node': data_rssi,
                        'rssi-gw': packet.rssi
                    }
                }]
                client.write_points(points)
                logging.info("Data sent to db!")


def signal_hook(*args):
    if shutdown_hook():
        logging.info("calling exit 0")
        sys.exit(1)


def shutdown_hook():
    global shutting_down
    if shutting_down:
        return False
    shutting_down = True
    logging.info("You are now leaving the Python sector - the app is being shutdown.")
    return True


def init_logging():
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)

    handler = logging.StreamHandler(sys.stdout)
    logger.addHandler(handler)
    
    log_address = '/var/run/syslog' if sys.platform == 'darwin' else '/dev/log'
    formatter = logging.Formatter('GW: %(message)s')
    handler = logging.handlers.SysLogHandler(address=log_address)
    handler.setFormatter(formatter)


if __name__ == '__main__':
    init_logging()
    try:
        main()
    except Exception as e:
        logging.exception(e)
        raise
