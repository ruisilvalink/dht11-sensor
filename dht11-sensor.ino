#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <dht11.h>

#define DHT11PIN 0

ESP8266WebServer server(80);
dht11 DHT11;

boolean setupCompleted = false;
boolean initialized = false;

String softApName = "DHT11-sensor";
String softApPassword = "";

String wifiName;
String wifiPassword;

String serverHost;
String serverPort;
String serverPath";
String serverToken;

const String homePage = "\
<html>\
  <head>\
    <title>dht11-sensor setup</title>\
  </head>\
  <body>\
    <h1>Setup</h1><br>\
    <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/setup\">\
      Wifi Name: <input type=\"text\" name=\"wifiName\"><br>\
      Wifi Passwordt: <input type=\"text\" name=\"wifiPassword\"><br>\
      Server Host: <input type=\"text\" name=\"serverHost\"><br>\
      Server Port: <input type=\"text\" name=\"serverPort\"><br>\
      Server Path: <input type=\"text\" name=\"serverPath\"><br>\
      Server Token: <input type=\"text\" name=\"serverToken\"><br>\
      <input type=\"submit\" value=\"Submit\">\
    </form>\
  </body>\
</html>";

void setup()
{
  Serial.begin(115200);
  Serial.println();

  boolean result = WiFi.softAP(softApName, softApPassword);
  Serial.println( result ? "Ready" : "Failed!");
  
  server.on("/", homeController);
  server.on("/setup", setupController);
  server.begin();
}

void loop()
{
  if (!setupCompleted) {
    server.handleClient();
  } else  if (!initialized){
    delay(5000);
    server.stop();
    connectWifi();
    initialized = true;
  } else {
    if (DHT11.read(DHT11PIN) == DHTLIB_OK) 
    {
      save(DHT11.humidity, DHT11.temperature);
    } else {    
      Serial.println("Sensor read error!");
    }
    delay(5000);
  }
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
    setupCompleted = true;
    server.send(200, "text/plain", "Setup Done");
  }
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiName, wifiPassword);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void save(float humidity, float temperature) 
{
  Serial.println();
  
  Serial.print("Humidity (%): ");
  Serial.println(humidity, 2);

  Serial.print("Temperature (C): ");
  Serial.println(temperature, 2);
    
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
