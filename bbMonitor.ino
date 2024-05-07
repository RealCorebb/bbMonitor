#include <Preferences.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

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
const int pins[] = {P1, P2, P3, P4, P5, P6, P7, P8};
int luminence = 128;

#define LINES 2
#define EACH_PIXEL_COUNT 2
int cols = sizeof(pins) / sizeof(pins[0]) / LINES;

#define STRIP1_PIN    39   // Example pin for strip 1, replace with your actual pin
#define STRIP2_PIN    40   // Example pin for strip 2, replace with your actual pin
#define PIXEL_COUNT   8  // Replace with the actual number of NeoPixels per strip

NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt1Ws2812xMethod> strip1(PIXEL_COUNT, STRIP1_PIN);
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt2Ws2812xMethod> strip2(PIXEL_COUNT, STRIP2_PIN);

NeoPixelAnimator animations1(PIXEL_COUNT);
NeoPixelAnimator animations2(PIXEL_COUNT);

AnimEaseFunction easing = NeoEase::CubicIn;

#define MAX 250

Preferences preferences;
bool mdnsOn = false;

AsyncWebServer server(80);
AsyncWebSocket ws("/");

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client) {
  AwsFrameInfo *info = static_cast<AwsFrameInfo*>(arg);
  static String message;

  if (info->index == 0) {
    // This is a new message, clear our message string
    message = "";
  }

  // Append the data from this frame to our message
  for(size_t i=0; i<len; i++) {
    message += (char)data[i];
  }

  if (info->final) {
    // The final frame has been received, process the message
    handleWebSocketText((uint8_t *)message.c_str(), message.length(),client);
  }
}

void handleWebSocketText(uint8_t * payload, size_t length, AsyncWebSocketClient *client) {
  // Parse JSON data
  if (memcmp(payload, "getConfig", sizeof("getConfig") - 1) == 0) {
     Serial.println("getConfig");
     String config_current = preferences.getString("config","{\"config\":{\"data\":[\"cpu_usage[0]\",\"cpu_usage[1]\",\"cpu_usage[2]\",\"cpu_usage[3]\",\"cpu_usage[4]\",\"cpu_usage[5]\",\"cpu_usage[6]\",\"cpu_usage[7]\"],\"brightNess\":128,\"smoothTime\":0}}");
     Serial.println(config_current);
     client->text(config_current);
  }
  else{
    DynamicJsonDocument jsonDoc(512);
    DeserializationError error = deserializeJson(jsonDoc, payload, length);

    if (error) {
      //Serial.println("Failed to parse JSON");
      return;
    }

    // Check if the "data" key exists
    if (jsonDoc.containsKey("data")) {
      JsonArray data = jsonDoc["data"];
      Serial.println(data);
      
      // Iterate through the array and set analogWrite values for defined pins
      for (int i = 0; i < data.size(); i++) {
        // Ensure pinIndex is within bounds
        if (i < sizeof(pins) / sizeof(pins[0])) {
          float pinValue = data[i].as<float>(); // Extract the value from the array element
          int pinIndex = i;
          
          analogWrite(pins[pinIndex], MAX * pinValue);
          
          Serial.print("Set PWM value for Pin ");
          Serial.print("P" + String(pinIndex + 1)); // Pin names start from P1
          Serial.print(" to ");
          Serial.println(pinValue);
          
          // Determine the strip and pixel index based on pinIndex
          int stripIndex = pinIndex < cols ? 0 : 1;
          int pixelIndex = (pinIndex & cols - 1) * EACH_PIXEL_COUNT;

          // Set NeoPixel animation based on pinValue
          setNeoPixelAnimation(stripIndex, pixelIndex, pinValue);
        } else {
          Serial.println("Invalid Pin Index: " + String(i));
        }
      }
    }
    else if(jsonDoc.containsKey("config")){
        //write the entire json.config string to the preferences config
        luminence = jsonDoc["config"]["brightNess"].as<int>();
        preferences.putString("config", jsonDoc["config"].as<String>());  
    }
    else {
        Serial.println("No 'data' key found in JSON");
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      ws.cleanupClients();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len, client);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void setup() {
  preferences.begin("bbMonitor", false);
  Serial.begin(115200);
  // put your setup code here, to run once:
  pinMode(P1,OUTPUT);
  pinMode(P2,OUTPUT);
  pinMode(P3,OUTPUT);
  pinMode(P4,OUTPUT);
  pinMode(P5,OUTPUT);
  pinMode(P6,OUTPUT);
  pinMode(P7,OUTPUT);
  pinMode(P8,OUTPUT);

  //Setup WiFi
  String ssid = preferences.getString("ssid","Hollyshit_A");
  String passwd = preferences.getString("passwd","00197633");
  String config = preferences.getString("config","{\"config\":{\"data\":[\"cpu_usage[0]\",\"cpu_usage[1]\",\"cpu_usage[2]\",\"cpu_usage[3]\",\"cpu_usage[4]\",\"cpu_usage[5]\",\"cpu_usage[6]\",\"cpu_usage[7]\"],\"brightNess\":128,\"smoothTime\":0}}");
  if(ssid != "null" && passwd != "null"){
    Serial.println("WiFi:");
    Serial.println(ssid);
    Serial.println(passwd);
    WiFi.begin(ssid.c_str(),passwd.c_str());
  }

  //Load Config
  DynamicJsonDocument jsonDoc(512);
  DeserializationError error = deserializeJson(jsonDoc, config.c_str());
  if (error) {
    Serial.println("Failed to parse JSON");
  }
  else{
    luminence = jsonDoc["config"]["brightNess"].as<int>();
  }

  // Setup LED
  strip1.Begin();
  strip2.Begin();
  RgbColor color = RgbColor(25, 25, luminence);
  for(int i = 0; i < PIXEL_COUNT; i ++) {
    strip1.SetPixelColor(i, color);
    strip2.SetPixelColor(i, color);
  }
  strip1.Show();
  strip2.Show();

  // Setup WebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.begin();

  Serial.println("bbMonitor Setup Done");
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
  animations1.UpdateAnimations();
  animations2.UpdateAnimations();
  strip1.Show();
  strip2.Show();
  if (Serial.available()) {
    // Read the serial data
    String serialData = Serial.readStringUntil('\n');
    
    // Parse JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, serialData);
    if (error) {
      Serial.println("Failed to parse JSON");
      return;
    }
    if (doc.containsKey("ssid") && doc.containsKey("password")) {
      // Extract SSID and password from JSON
      const char* ssid = doc["ssid"];
      const char* password = doc["password"];
      
      // Store SSID and password in preferences
      preferences.putString("ssid", ssid);
      preferences.putString("passwd", password);
      
      // Connect to WiFi
      Serial.print("Connecting to new WiFi...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      
    }
  }
}

void setNeoPixelAnimation(int stripIndex, int pixelIndex, float pinValue) {
  // Define animation parameters
  RgbColor green(0, luminence, 0);
  RgbColor red(luminence, 0, 0);
  
  // Calculate color based on pinValue
  RgbColor targetColor = RgbColor::LinearBlend(green, red, pinValue);
  // Set animation
  if (stripIndex == 0) {
    AnimUpdateCallback animUpdate = [=](const AnimationParam& param)
      {
          float progress = easing(param.progress);
          RgbColor color = RgbColor::LinearBlend(strip1.GetPixelColor(pixelIndex), targetColor, progress);
          for(int i = 0; i < EACH_PIXEL_COUNT; i ++) {
            strip1.SetPixelColor(pixelIndex + i, color);
          }
      };
    animations1.StartAnimation(pixelIndex, 1000, animUpdate);
  } else {
    animations2.StartAnimation(pixelIndex, 1000, [targetColor, pixelIndex](const AnimationParam& param) {
      RgbColor color = RgbColor::LinearBlend(strip2.GetPixelColor(pixelIndex), targetColor, param.progress);
      for(int i = 0; i < EACH_PIXEL_COUNT; i ++) {
        strip2.SetPixelColor(pixelIndex + i, color);
      }
    });
  }
}
