from RFM69 import Radio, FREQ_433MHZ
from influxdb import InfluxDBClient
import atexit

import datetime
import json
import logging
import logging.handlers
import os
import sdnotify
import signal
import sys
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
                logging.debug("Raw packet data: %s", packet);
                data = bytearray(packet.data).decode()
                data_dict = dict(x.split(':') for x in data.split(','))
                points = [{
                    'measurement': 'readings',
                    'time': datetime.datetime.utcnow(),
                    'tags': {
                        'network': network_id,
                        'node': packet.sender,
                        'name': data_dict.get('nm', str(packet.sender))
                    },
                    'fields': {
                        'temp': int(data_dict.get('t', 0)) / 10,  # temp was *10 to avoid doing float -> str on arduino side
                        'pressure': int(data_dict.get('p', 0)),
                        'humidity': float(data_dict.get('h', 0)),
                        'lumi': int(data_dict.get('l', -1)),
                        'voltage': int(data_dict.get('v')) / 1000,  # mV to V
                        'rssi-node': int(data_dict.get('r'), 0),  # last known RSSI as seen on node side
                        'rssi-gw': packet.RSSI
                    }
                }]
                client.write_points(points)
                logging.debug("Decoded data: %s", json.dumps(points, default=str))
                logging.info("Readings data sent to db!")


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
