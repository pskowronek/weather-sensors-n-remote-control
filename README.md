_Language versions:_

[![EN](https://github.com/pskowronek/weather-sensors-n-remote-control/raw/main/www/flags/lang-US.png)](https://github.com/pskowronek/weather-sensors-n-remote-control) 
[![PL](https://github.com/pskowronek/weather-sensors-n-remote-control/raw/main/www/flags/lang-PL.png)](https://translate.googleusercontent.com/translate_c?sl=en&tl=pl&u=https://github.com/pskowronek/weather-sensors-n-remote-control)
[![DE](https://github.com/pskowronek/weather-sensors-n-remote-control/raw/main/www/flags/lang-DE.png)](https://translate.googleusercontent.com/translate_c?sl=en&tl=de&u=https://github.com/pskowronek/weather-sensors-n-remote-control)
[![FR](https://github.com/pskowronek/weather-sensors-n-remote-control/raw/main/www/flags/lang-FR.png)](https://translate.googleusercontent.com/translate_c?sl=en&tl=fr&u=https://github.com/pskowronek/weather-sensors-n-remote-control)
[![ES](https://github.com/pskowronek/weather-sensors-n-remote-control/raw/main/www/flags/lang-ES.png)](https://translate.googleusercontent.com/translate_c?sl=en&tl=es&u=https://github.com/pskowronek/weather-sensors-n-remote-control)

# Weather sensors & remote control

The project is to measure temperature, athmospheric air pressure, humidity and luminosity by using arduino-powered sensors. Sensors sent their data every ~15m (configurable) by using RFM69 modules and collected by using RPi. RPi is also used to visualize data by using Grafana and InfluxDB.

The project consists of two main parts (folders):

- arduino - two Arduino Projects:
	- WeatherNode - a code for sensor and control (optionally)
	- TriggerNode - a code for triggerring/control something via WeatherNode
- rpi - installation scripts and python program (that can be set as a Service) to collect data from arduino sensors

## Hardware

List of parts you will need:
- Arduino Mini Pro or similar (preferred 3V 8MHz for battery life-span)
- RPi ZeroW (or similar)
- RFM69 modules for 433MHz connectivity
- BME280 modules for temp, humidity & atm pressure
- TSL2561 modules for luminosity (optional)
- a couple of wires, battery housing etc

## Software

List of software/libraries you will need:
- [Arduino IDE](https://www.arduino.cc/en/software)
- libraries for WeatherNode & TiggerNode (via Arduino library): RFM69, SparkFunBME280, Adafruit_TSL2561, LowPower

## Wiring

For guidence how to connect modules to Arduino or RPi refer to [arduino/WeatherNode/WeatherNode.ino](https://github.com/pskowronek/weather-sensors-n-remote-control/blob/main/arduino/WeatherNode/WeatherNode.ino) and this [page](https://rpi-rfm69.readthedocs.io/en/latest/hookup.html).

If you intend to use batteries to power Arduino sensors then you need to modify your Arduino to lower current consumption - removal of power LED is a must. Use Vcc pin to provide 3V directly (if you plan to use 2xAAs).
To further extend battery life you may want to modify fuses on your Arduino so it could run below 2.8V (even as low as 1.8V).

## Screenshots / Photos

### Screenshots
![Screenshots](https://github.com/pskowronek/weather-sensors-n-remote-control/raw/main/www/screenshots/grafana.jpg)


### Photos
[![Assembled](https://github.com/pskowronek/weather-sensors-n-remote-control/raw/main/www/assembled/03.jpg)](https://pskowronek.github.io/weather-sensors-n-remote-control/www/assembled/index.html "Photos of assembled sensors, triggers etc")

More photos of the assembled sensors are [here](https://pskowronek.github.io/weather-sensors-n-remote-control/www/assembled/index.html "Photos of assembled sensors, triggers etc").


## License

The code is licensed under Apache License 2.0, pictures under Creative Commons BY-NC.

## Authors

- [Piotr Skowronek](https://github.com/pskowronek)
