#!/bin/bash

echo "Going to update the system first..."
sudo apt update
sudo apt upgrade

echo "Going to install required tools..."
sudo apt install wget
sudo apt install curl

echo "Going to install Grafana from deb package..."
# taken from: https://grafana.com/grafana/download?platform=arm
sudo apt install adduser libfontconfig1 -y
rm /tmp/grafana.deb 2> /dev/null
curl -L https://dl.grafana.com/oss/release/grafana-rpi_7.3.6_armhf.deb -o /tmp/grafana.deb
sudo dpkg -i /tmp/grafana.deb

sudo mv /etc/grafana/grafana.ini /etc/grafana/grafana.ini-ORG
sudo cp dockerfiles/grafana/grafana.ini /etc/grafana/grafana.ini

echo "Going to configure Grafana to start at boot-time..."
sudo /bin/systemctl daemon-reload
sudo /bin/systemctl enable grafana-server
sudo /bin/systemctl start grafana-server

echo "Going to to install docker..."
rm /tmp/get-docker.sh 2> /dev/null
curl -L https://get.docker.com -o /tmp/get-docker.sh
sh /tmp/get-docker.sh
sudo usermod -aG docker pi

echo "Going to install docker-compose..."
sudo apt install python3-pip -y
sudo pip3 install docker-compose

echo "Going to install gateway.py requirements..."
pip3 install -r requirements.txt

echo "Now you must re-login to update user groups (you may also want to reboot) and run 02_setup.sh then."

