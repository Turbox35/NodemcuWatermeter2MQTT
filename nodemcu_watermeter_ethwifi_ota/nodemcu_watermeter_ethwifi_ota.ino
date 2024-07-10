#include <ArduinoOTA.h>

#include <ESP8266WiFi.h>
#include <W5500lwIP.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

//set the network to 1 for ethernet or 2 for wifi
const int setnetwork = 1;

//wifi settings
const char* WIFI_SSID = "YOURWIFISSID";
const char* WIFI_PASSWORD = "YOURWIFIPASSWORD";

//ethernet settings
#define CSPIN 16
Wiznet5500lwIP eth(CSPIN);
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};//change it if you have multiple devices

//MQTT settings
const char* mqttServer = "192.168.X.X"; // The IP of your MQTT broker
const int mqttPort = 1883; //The port of your MQTT broker
const char* mqttUser = ""; //The user of your MQTT broker
const char* mqttPassword = ""; // The password associated to the user
const char* autodiscoveryTopic = "homeassistant"; //The auto discovery topic
const char* topic = "nodemcudata"; //The MQTT topic
const int availabilitydelay = 60000; //set time in milisecond of the availability mqtt publish

//sensor settings
int sensorNumber = 1; //change the sensor number if you have multiple device using this code
const char* deviceName = "watermeter";

//-----end of personnal settings

const char* payloadTemp = "";
char buffer[8];
bool ini;
int status = WL_IDLE_STATUS;

bool binaryinputStatus;
int binary = 1;

int input_pin = D2;

int inputStatusLast = LOW;  // last status switch
int LEDStatus = LOW;         // current status LED
int availabilitycounter = 600000;

String mqttName = String(deviceName) + " " + String(sensorNumber);
String stateTopic = String(topic) + "/device/" + String(sensorNumber) + "/state";
String setTopic = String(topic) + "/device/" + String(sensorNumber) + "/set";
String availabilityTopic = String(topic) + "/device/" + String(sensorNumber) + "/available";

int counter;
int present;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void connectethernet(){
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setFrequency(4000000);
  eth.setDefault(); // use ethernet for default route
  present = eth.begin(mac);
  Serial.println("present check");
  if (!present) {
    Serial.println("no ethernet hardware present");
    while(1);
  } 
  Serial.print("connecting ethernet");
  while (!eth.connected()) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  Serial.print("ethernet ip address: ");
  Serial.println(eth.localIP());
  Serial.print("ethernet subnetMask: ");
  Serial.println(eth.subnetMask());
  Serial.print("ethernet gateway: ");
  Serial.println(eth.gatewayIP());
}

void connectwifi(){
  //WiFi.mode(WIFI_STA); //optional
  WiFi.setPhyMode(WIFI_PHY_MODE_11G); //optional
  delay(500); 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
  Serial.println(WIFI_SSID);
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) {
    //delay(500); optional
    Serial.print(".");
    //Serial.println(WiFi.status()); optional
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  Serial.println("Connected to Wi-Fi");
  Serial.println(WiFi.localIP());
}

void setup() {
  delay(5000);
  ini = true;
  Serial.begin(9600);
  Serial.println();
  Serial.print(ESP.getFullVersion());
  Serial.println();
  pinMode(BUILTIN_LED, OUTPUT);

  // ethernet code ------------------------------------------
  if (setnetwork == 1){
    //connect ethernet
    connectethernet();
  } else if (setnetwork == 2){
    //connect to wifi
    connectwifi();
  } else {
    Serial.println("Error in configuration, set network to wifi or ethernet");
    while(true){} //infinite loop which will stop the program
  }

  delay(1000);
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  
  Serial.println("Connecting to MQTT");
  while (!client.connected()) {
    Serial.print(".");
    if (client.connect(mqttName.c_str(), mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT");
      client.subscribe(setTopic.c_str());
      digitalWrite(BUILTIN_LED, HIGH);  // turn off LED with voltage LOW
      client.setBufferSize(1024);
      sendMQTTDiscoveryMsg();
    } else {
      Serial.println("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  // Initialiser l'OTA
  ArduinoOTA.setPassword("watermeter");
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    // NOTE: si vous utilisez SPIFFS, évitez de formater le système de fichiers pendant la mise à jour
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  //end ota code
}

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  bool verifdigit = true;
  //Serial.print("Message:"); //for debug
  for (int i = 0; i < length; i++)
  {
    //Serial.print((char)payload[i]); //for debug
    payloadTemp += (char)payload[i];
    //Serial.print(payloadTemp); //for debug
    //Serial.println(); //for debug
    buffer[i] = (char)payload[i];

  }
  if (String(topic) == setTopic) {
    for(int i = 0;buffer[i] != '\0'; i++)
    {
      if (!isDigit(buffer[i])) {  // tests if myChar is a digit
        Serial.println("detected non digit value");
        verifdigit = false;
        break;
      }
    else {
      Serial.println(buffer[i]);
      }
    }
    if (verifdigit) {
      counter = atoi(buffer);
      Serial.println("new counter value accepted");
    }
    Serial.print(counter);
    Serial.println();
  }
}

void sendMQTTDiscoveryMsg() {
  String discoveryTopic = String(autodiscoveryTopic) + "/sensor/" + String(deviceName) + String(sensorNumber) + "/device/config";
  const int expireafter = availabilitydelay *0.002;

  char output[1024];
  size_t outputCapacity;
  JsonDocument doc;

  doc["name"] = String(deviceName) + " " + String(sensorNumber) + " sensor";
  doc["state_topic"] = stateTopic;
  doc["unique_id"] = String(deviceName) + " " + String(sensorNumber) + " sensor";
  doc["command_topic"] = setTopic;
  doc["availability_topic"] = availabilityTopic;
  doc["device_class"] = "water";
  doc["value_template"] = "{{ value_json.value }}";
  doc["unit_of_meas"] = "L";
  doc["state_class"] = "total_increasing";
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  //doc["expire_after"] = expireafter;
  doc["availability_mode"] = "any";

  JsonObject device = doc["device"].to<JsonObject>();
  device["identifiers"][0] = "watermeter_1";
  device["manufacturer"] = "nodemcu";
  device["model"] = "nodemcu LJ18A3-8-Z";
  device["name"] = "watermeter";
  device["sw_version"] = "1.0";

  doc.shrinkToFit();  // optional

  serializeJson(doc, output);
  client.publish(discoveryTopic.c_str(), output, true);
}

void mqttconnect(){
  client.setServer(mqttServer, mqttPort);
  
  Serial.println("Connecting to MQTT");

  while (!client.connected()) {
    Serial.print(".");
    delay(100);
    if (client.connect(mqttName.c_str(), mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT");
      client.subscribe(setTopic.c_str());
    }
  }
}

void loop() {
  delay(10);
  ArduinoOTA.handle();
  if (WiFi.status() == WL_CONNECTED || eth.connected()) {
    if ((availabilitycounter *10) > availabilitydelay){
      //publish 
      client.publish(availabilityTopic.c_str(), "online");
      Serial.println("Online published to MQTT broker");
      availabilitycounter = 0;
    }
    availabilitycounter++;
	  pinMode(input_pin, INPUT);

	  int inputStatus = digitalRead(input_pin);   // read status of switch
	  if (inputStatus != inputStatusLast)  // if status of button has changed
      {
        Serial.println("wait debouncing");
        delay(100);     // debounce time of 50 ms
        inputStatus = digitalRead(input_pin);   // read the status of the switchPIN again
        Serial.println("inputStatus après relecture");
        Serial.println(inputStatus);
        if (inputStatus != inputStatusLast) // if the status of the button has changed again
        {
          //inputStatus = inputStatusLast;
          if (inputStatus == binary)
          {
            Serial.println("detected pulse low");
            binaryinputStatus = true;
            inputStatusLast = inputStatus;
          }
          else {
            Serial.println("detected pulse high");
            binaryinputStatus = false;
            inputStatusLast = inputStatus;
            if (!ini)
            {
              counter = counter + 1;
            }
          }
		      if (!ini)
          {
            DynamicJsonDocument doc(1024);
		        char buffer[512];
            doc["value"] = counter;
            doc["nodeName"] = String(deviceName) + " " + String(sensorNumber) + " sensor";
            size_t n = serializeJson(doc, buffer);
            //Serial.println(buffer); //for debug
            //Serial.println(n); //for debug
            client.setServer(mqttServer, mqttPort);
            bool published = client.publish(stateTopic.c_str(), buffer);
            //Serial.println("published: "); //for debug
            //Serial.println(published); //for debug
            Serial.println("counter published: ");
            Serial.println(counter);
            inputStatusLast = inputStatus;
            if (published == 0)
            {
              Serial.println("not published");
              String temp;
              temp = client.connect(mqttName.c_str(), mqttUser, mqttPassword);
              Serial.println("etat de la connexion MQTT");
              Serial.println(temp);
              mqttconnect();
            }
            char msg_out[8];
            sprintf(msg_out, "%d",counter);
            published = client.publish(setTopic.c_str(), msg_out,true);
            //Serial.println("backup published: "); //for debug
            //Serial.println(published); //for debug
          }
          else{
            ini = false;
            inputStatusLast = inputStatus;
          }
        }
      }    
  }
  else {
    Serial.println("WiFi/Ethernet Disconnected");
    delay(10000);
    if (setnetwork == 1){
      Serial.println("Try to reconnect to Ethernet");
      //connect ethernet
      connectethernet();
    } else if (setnetwork == 2){
      Serial.println("Try to reconnect to wifi");
      //connect to wifi
      connectwifi();
    }
  }
  client.loop();
}