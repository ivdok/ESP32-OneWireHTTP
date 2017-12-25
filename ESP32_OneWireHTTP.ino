#include <ESP32WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiClient.h>

#define dsPin 15

OneWire oneWire(dsPin); //oneWire bus device init
DallasTemperature sensors(&oneWire); //sensor init

const char* ssid     = "ssid";
const char* password = "your_password";
// int _conntime = 0; //you may not need this

ESP32WebServer server(80);

void setup()
{
  Serial.begin(115200);
  sensors.begin();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {	/*
    delay(1);					* This servers only as a inefficiently implemented benchmark for testing AP responsiveness, safe to ignore
    _conntime++;				*/
  }						

  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
  if (MDNS.begin("esp32")) {
    Serial.println("mDNS responder started");
  }
  else {
    Serial.println("mDNS responder failure");
  }						/* ^ Read this ^
  Serial.print("Connection took ~");		*
  Serial.print(_conntime);			*
  Serial.println("ms");				*/

  server.on("/", handleRoot); // This is simple, alright
  server.on("/temperature", []() { //Now some inline bullshit happens because normal function calls are failing, dunno why
    Serial.println("Requesting temperatures...");
    String httpResponder = ""; //Dark magic to reduce chance of WiFi/server crash
    portDISABLE_INTERRUPTS(); //https://github.com/espressif/arduino-esp32/issues/755
    sensors.requestTemperatures(); //Gimme dat data!
    httpResponder = String(sensors.getTempCByIndex(0));//Implementing others should only take indexing sensors and storing addresses, look up Examples > DallasTemperature
    portENABLE_INTERRUPTS(); 
    Serial.print("Temperature C: ");
    Serial.println(httpResponder);
    //Blasphemy below made with http://tomeko.net/online_tools/cpp_text_escape.php?lang=en
    httpResponder = String("<!DOCTYPE html>\n<html lang=\"ru-RU\" dir=\"ltr\">\n<head>\n<meta charset=\"utf-8\">\n<meta http-equiv=\"X-UA-Compatible\" content=\"IE=Edge\"><meta http-equiv=\"refresh\" content=\"5\">\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n<meta name=\"robots\" content=\"nofollow\">\n<body>\n<center><h1><b>Temp C: ") + String(httpResponder) + String("</b></h1></center>\n</body>\n</html>\n");
    /*That string up there, it hurts to edit. If you need large bulk HTML, use C++11 raw strings literals:
    *const char INDEX_HTML[] = R"(
    *<html>YOUR STUFF</html>
    *)";
    *Also, link below looks promising, but I didn't examine it throughly yet:
    *https://diyprojects.io/esp8266-web-server-part-1-store-web-interface-spiffs-area-html-css-js/
    */
    server.send(200, "text/html", httpResponder);
  });

  server.onNotFound(handleNotFound); //404 yada yada
  server.begin();
  Serial.println("HTTP server started");

}

void loop() {
  server.handleClient();
}

void handleRoot() {
  server.send(200, "text/plain", "Hello from ESP32!");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}