#include <LittleFS.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <dht11.h>
#include "FS.h"

const int DHT11_PIN = 0; // ESP8666 pin D3
const int ON_SWITCH_PIN = 5; // ESP8666 pin D1

int onSwitchState = 0; // Start in setup mode

ESP8266WebServer server(80);
dht11 DHT11;

String softApName = "DHT11-sensor";
String softApPassword = "";

String wifiName;
String wifiPassword;

String serverHost;
String serverPort;
String serverPath;
String serverToken;

const String homePage = "\
<html>\
  <head>\
    <title>dht11-sensor setup</title>\
    <style>\
      label {\
        display: block;\
        margin-top: 10px;\
      }\
    </style>\
  </head>\
  <body>\
    <h1>Setup</h1><br>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/setup\">\
      <label>Wifi Name:</label><input type=\"text\" name=\"wifiName\"><br>\
      <label>Wifi Password:</label><input type=\"password\" name=\"wifiPassword\"><br>\
      <label>Server Host:</label><input type=\"text\" name=\"serverHost\"><br>\
      <label>Server Port:</label><input type=\"text\" name=\"serverPort\"><br>\
      <label>Server Path:</label><input type=\"text\" name=\"serverPath\"><br>\
      <label>Server Token:</label><input type=\"text\" name=\"serverToken\"><br>\
      <br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
  </body>\
</html>";

void setup()
{
  Serial.begin(115200);
  delay(3000);
  Serial.println();
  
  LittleFS.begin();

  loadFSConfig();

  pinMode(ON_SWITCH_PIN, INPUT);

  startWifiAP();
  
  server.on("/", homeController);
  server.on("/setup", setupController);
  server.begin();
}

void loop()
{
  int currentOnSwitchState = digitalRead(ON_SWITCH_PIN);
  if (currentOnSwitchState == HIGH) {
    if (currentOnSwitchState != onSwitchState) {
      startWifiSTA();
    }
    readSensor();
  } else {
    if (currentOnSwitchState != onSwitchState) {
      startWifiAP();
    }
    server.handleClient();
  }
  onSwitchState = currentOnSwitchState;
}

void loadFSConfig() {
  File file = LittleFS.open("/config.txt", "r");
  if (!file) {
    Serial.println("Config not found.");
    return;
  }
  wifiName = file.readStringUntil('\n');
  wifiPassword = file.readStringUntil('\n');
  serverHost = file.readStringUntil('\n');
  serverPort = file.readStringUntil('\n');
  serverPath = file.readStringUntil('\n');
  serverToken = file.readStringUntil('\n');
  Serial.println("wifiName: " + wifiName);
  Serial.println("wifiPassword: " + wifiPassword);
  Serial.println("serverHost: " + serverHost);
  Serial.println("serverPort: " + serverPort);
  Serial.println("serverPath: " + serverPath);
  Serial.println("serverToken: " + serverToken);
}

void saveFSConfig() {
  File file = LittleFS.open("/config.txt", "w");
  if (!file) {
    Serial.println("Config not found.");
    return;
  }
  file.print(wifiName + '\n');
  file.print(wifiPassword + '\n');
  file.print(serverHost + '\n');
  file.print(serverPort + '\n');
  file.print(serverPath + '\n');
  file.print(serverToken + '\n');
}

void homeController() {
  if (server.method() != HTTP_GET) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    server.send(200, "text/html", homePage);
  }
}

void setupController() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    wifiName = server.arg("wifiName");
    wifiPassword = server.arg("wifiPassword");
    serverHost = server.arg("serverHost");
    serverPort = server.arg("serverPort");
    serverPath = server.arg("serverPath");
    serverToken = server.arg("serverToken");
    saveFSConfig();
    server.send(200, "text/plain", "Setup Done");
  }
}

void startWifiAP() {
  WiFi.disconnect(true);
  
  boolean result = WiFi.softAP(softApName, softApPassword);
  Serial.println( result ? "Ready" : "Failed!");
}

void startWifiSTA() {
  WiFi.softAPdisconnect(true);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiName, wifiPassword);

  Serial.print("Connecting");
  int timeout = 10000;
  while (WiFi.status() != WL_CONNECTED && timeout > 0)
  {
    int waitTime = 1000;
    delay(waitTime);
    timeout -= waitTime;
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Not connected!");
  } else {
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void readSensor() {
  if (DHT11.read(DHT11_PIN) == DHTLIB_OK) 
  {
    Serial.println();
    
    Serial.print("Humidity (%): ");
    Serial.println(DHT11.humidity, 2);
  
    Serial.print("Temperature (C): ");
    Serial.println(DHT11.temperature, 2);
    
    save(DHT11.humidity, DHT11.temperature);
  } else {    
    Serial.println("Sensor read error!");
  }
  delay(5000);
}

void save(float humidity, float temperature) 
{
  String content = "token=" + serverToken + "&humidity=" + String(humidity) + "&temperature=" + String(temperature);
  
  WiFiClient client;
  if (!client.connect(serverHost, serverPort.toInt())) {
    Serial.println("Could not connect!");
    return;
  }
  client.println("POST " + serverPath + " HTTP/1.1");
  client.println("Host: " + serverHost);
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.println("Content-Length: " + String(content.length()));
  client.println();
  client.println(content);
  client.stop();
}
