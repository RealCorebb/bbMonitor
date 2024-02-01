#include <Preferences.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

#define P1 38
#define P2 5
#define P3 6
#define P4 7
#define P5 15
#define P6 16
#define P7 17
#define P8 18
#define P9 8
#define P10 9
#define P11 10
#define P12 11
#define P13 12
#define P14 13
#define P15 14
#define P16 21
#define P17 35
#define P18 36
const int pins[] = {P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13, P14, P15, P16, P17, P18};

#define MAX 250

Preferences preferences;
bool mdnsOn = false;

WebSocketsServer webSocket = WebSocketsServer(80);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_TEXT:
      handleWebSocketText(payload, length);
      break;
  }
}

void handleWebSocketText(uint8_t *payload, size_t length) {
  // Parse JSON data
  DynamicJsonDocument jsonDoc(512);
  DeserializationError error = deserializeJson(jsonDoc, payload, length);

  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }

  // Check if the "data" key exists
  if (jsonDoc.containsKey("data")) {
    JsonObject data = jsonDoc["data"];

    // Iterate through keys in "data" and set analogWrite values for defined pins
    for (JsonPair kv : data) {
      String pinName = kv.key().c_str();

      // Find the corresponding pin based on the key
      int pinIndex = pinName.substring(1).toInt() - 1;

      // Check if the pin index is within the valid range
      if (pinIndex >= 0 && pinIndex < sizeof(pins) / sizeof(pins[0])) {
        int pinValue = kv.value();
        analogWrite(pins[pinIndex], pinValue);
        Serial.print("Set PWM value for Pin ");
        Serial.print(pinName);
        Serial.print(" to ");
        Serial.println(pinValue);
      } else {
        Serial.println("Invalid Pin: " + pinName);
      }
    }
  } else {
    Serial.println("No 'data' key found in JSON");
  }
}

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  pinMode(P1,OUTPUT);
  pinMode(P2,OUTPUT);

  //Setup WiFi
  String ssid = preferences.getString("ssid","Hollyshit_A");
  String passwd = preferences.getString("passwd","00197633");
  if(ssid != "null" && passwd != "null"){
    WiFi.begin(ssid.c_str(),passwd.c_str());
  }
}

void setupMDNS(){
  if (!MDNS.begin("bbMonitor")) {
        Serial.println("Error setting up MDNS responder!");
    }
    else{
      MDNS.addService("bbMonitor", "tcp", 80);
      mdnsOn = true;
    }
}

void loop() {
  // put your main code here, to run repeatedly:
  if(!mdnsOn) setupMDNS();
  webSocket.loop();
}
