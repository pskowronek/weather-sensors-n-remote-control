#!/bin/bash

echo "Please test if gateway works - prints on console received data and successfuly sends them out to InfluxDB"
echo "If not, then you need to either edit gateway.py and its configuration or debug it more deeply :)"
echo
echo "Going to run gateway.py..."
python3 gateway.py

exit $?
