#!/bin/bash

# Authentication in InfluxDB docker image is disabled (this version apparently cannot automatically create users from env settings)
# If you decide to use authentication then edit creds below.
ADMIN_USER_NAME='admin'
ADMIN_USER_PASSWD='changeme'
USER_NAME='user'
USER_PASSWD='changeme'

DATABASE_NAME='sensors'
DATABASE_ROW_NAME='readings'

echo "Going to install docker image with InfluxDB..." 
cd dockerfiles
docker-compose up --build -d
cd -

echo "Let's wait 10s for InfluxDB to start up..."
sleep 10s
echo "Going to init db..."
curl -XPOST "http://localhost:8086/query?u=$ADMIN_USER_NAME&p=$ADMIN_USER_PASSWD" --data-urlencode "q=CREATE DATABASE "$DATABASE_NAME""
curl -XPOST "http://localhost:8086/query?u=$ADMIN_USER_NAME&p=$ADMIN_USER_PASSWD" --data-urlencode 'q=SHOW DATABASES'
curl -XPOST "http://localhost:8086/query?db=$DATABASE_NAME&u=$USER_NAME&p=$USER_PASSWD" --data-urlencode "q=select * from $DATABASE_ROW_NAME"

echo "InfluxDB should be up and ready"
echo "If you need to migrate InfluxDB data then the database files are kept here: /var/data/influxdb"
echo
echo "Now open your favorite browser and navigate to: http://[rpi-ip]:3000/ and configure datasource and set up your dashboards"
echo "If you need to reconfigure Grafana, then edit /etc/grafana/grafana.ini and restart the service using: sudo systemctl restart grafana-server

