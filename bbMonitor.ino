#include <Preferences.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#define P1 38
#define P2 5
#define MAX 250

Preferences preferences;
bool mdnsOn = false;


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
  Serial.println(mdnsOn);
  if(!mdnsOn) setupMDNS();
  analogWrite(P1,MAX * 1);
  delay(1000);
  analogWrite(P1,MAX * 0.5);
  delay(1000);
  analogWrite(P1,MAX * 0);
  delay(1000);
  analogWrite(P1,MAX * 0.5);
  delay(1000);
}
