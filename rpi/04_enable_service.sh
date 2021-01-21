#!/bin/bash

echo "Going to copy gateway.service to /etc/systemd/system..."

sudo cp *.service /etc/systemd/system/

echo "Service has just been copied. You can start or enable/disable it anytime by invoking:"
echo "sudo systemctl [start|enable|disable] gateway.service"
echo

echo "Going to enable gateway.service to start on boot time..."
sudo systemctl enable gateway.service

echo "Done. You can restart RPi to see if it works!"
