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
# Good for Raspbian Buster, no good for Raspbian 11 Bullseye and/or RPi Zero:
#sudo apt install python3-pip -y
#sudo pip3 install docker-compose

# Good for Buster and RPi Zero
DOCKER_COMPOSE_VERSION="2.1.1"
DOCKER_COMPOSE_ARCH="armv6" # Good for RPi Zero, for others armv7 should be good
sudo curl -L "https://github.com/docker/compose/releases/download/v${DOCKER_COMPOSE_VERSION}/docker-compose-linux-${DOCKER_COMPOSE_ARCH}" -o /usr/bin/docker-compose
sudo chmod +x /usr/bin/docker-compose

echo "Going to install gateway.py requirements..."
pip3 install -r requirements.txt

echo "Now you must re-login to update user groups (you may also want to reboot) and run 02_setup.sh then."

