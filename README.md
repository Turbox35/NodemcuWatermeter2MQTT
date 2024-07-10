
# Watermeter reader wifi/ethernet

I know there is a lot of esp code which allow to catch watermeter pulse but this one is a little bit different.

My watermeter is far from my house and under the ground. With the use of an ethernet cable with poe support, it now possible for me.



## Features

- Connected to your network using ethernet or wifi.
- Send data to MQTT broker
- Send the exact index and not only a pulse
- Set index using MQTT (over the air index update)
- Autodiscovery for Domoticz or Home Assistant
- OTA support for programming

## Hardware

- Nodemcu V3
- W5500 Ethernet shield lan
- LJ18A3-8-Z M18 inductive sensor 5V
- micro usb poe splitter (if you need poe support)
- some 3D printed parts for the box and sealing (not mandatory)

## Wiring

In progress

## Pictures

https://github.com/Turbox35/NodemcuWatermeter2MQTT/blob/main/pictures/picture1.jpg


## How to deploy

Use Arduino IDE, install ArduinoJson (7.0.4 minimum) and PubSubClient (2.8) libraries.

Select NodeMCU 0.9 board

Modify the parameter at the beginning of the code:

- Select network
```bash
  const int setnetwork = 1;
```
set value to 2 if you use wifi


- Wifi setting

```bash
  const char* WIFI_SSID = "YOURWIFISSID"
  const char* WIFI_PASSWORD = "YOURWIFIPASSWORD"
```
ignore if you use ethernet connection

- Ethernet setting

```bash
  #define CSPIN 16
```
W5500 connection

```bash
  byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02}
```
Modify it you have an issue or multiple similar devices

- MQTT setting

```bash
  const char* mqttServer = "192.168.3.9"; // The  IP of your MQTT broker
  const int mqttPort = 1883; //The port of your MQTT broker
  const char* mqttUser = "admin"; //The user of your MQTT broker
  const char* mqttPassword = "mosquitto35*"; // The password associated to the user
  const char* autodiscoveryTopic = "homeassistant"; //The auto discovery topic
  const char* topic = "nodemcudata"; //The MQTT topic
  const int availabilitydelay = 60000; //set time in milisecond of the availability mqtt publish
```
adapt to your MQTT broker

- Sensor setting

```bash
  int sensorNumber = 1; //change the sensor number if you have multiple device using this code
  const char* deviceName = "watermeter";
```
Important to change if you have multiple device with the same code on your network
## License

[MIT](https://choosealicense.com/licenses/mit/)


## FAQ

#### How to set the new index over the air?

In progress

#### How to update firmware using OTA?

In progress

