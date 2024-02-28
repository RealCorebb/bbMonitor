#include <Preferences.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WebSocketsServer.h> //https://github.com/Links2004/arduinoWebSockets
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
// Map pinIndex to NeoPixel strips and pixel index
int pinToPixelMap[] = {
  0, 1,  // strip 1, P1
  2, 3,  // strip 1, P2
  4, 5,  // strip 1, P3
  6, 7,  // strip 1, P4
  8, 9,  // strip 1, P5
  10, 8,  // strip 1, P6
  9, 10, // strip 1, P7
  10, 11,// strip 1, P8
  12, 13,// strip 1, P9

  0, 1,  // strip 2, P10
  2, 3,  // strip 2, P11
  4, 5,  // strip 2, P12
  6, 7,  // strip 2, P13
  8, 9,  // strip 2, P14
  10, 11,// strip 2, P15
  12, 13 // strip 2, P16
};

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
 else {
    Serial.println("No 'data' key found in JSON");
  }
}

void setup() {
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
  if(ssid != "null" && passwd != "null"){
    WiFi.begin(ssid.c_str(),passwd.c_str());
  }

  // Setup LED
  strip1.Begin();
  strip2.Begin();
  RgbColor color = RgbColor(25, 25, 255);
  for(int i = 0; i < PIXEL_COUNT; i ++) {
    strip1.SetPixelColor(i, color);
    strip2.SetPixelColor(i, color);
  }
  strip1.Show();
  strip2.Show();

  // Setup WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
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
  animations1.UpdateAnimations();
  animations2.UpdateAnimations();
  strip1.Show();
  strip2.Show();
}

void setNeoPixelAnimation(int stripIndex, int pixelIndex, float pinValue) {
  // Define animation parameters
  RgbColor green(0, 255, 0);
  RgbColor red(255, 0, 0);
  
  // Calculate color based on pinValue
  RgbColor targetColor = RgbColor::LinearBlend(green, red, pinValue);
  // Set animation
  if (stripIndex == 0) {
    AnimUpdateCallback animUpdate = [=](const AnimationParam& param)
      {
          float progress = easing(param.progress);
          RgbColor color = RgbColor::LinearBlend(strip1.GetPixelColor(pixelIndex), targetColor, progress);
          for(int i = 0; i < EACH_PIXEL_COUNT; i ++) {
            Serial.println(pixelIndex + i);
            strip1.SetPixelColor(pixelIndex + i, color);
          }
      };
    animations1.StartAnimation(pixelIndex, 1500, animUpdate);
  } else {
    animations2.StartAnimation(pixelIndex, 1500, [targetColor, pixelIndex](const AnimationParam& param) {
      RgbColor color = RgbColor::LinearBlend(strip2.GetPixelColor(pixelIndex), targetColor, param.progress);
      for(int i = 0; i < EACH_PIXEL_COUNT; i ++) {
        strip2.SetPixelColor(pixelIndex + i, color);
      }
    });
  }
}
