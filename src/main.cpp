
#include "Arduino.h"
#include "DHTesp.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>   


bool alarmActivated = false;
const int soundPin = 4;
const int ledPin = 2;
const int sensorPin = 14;
const float alarmHumidity = 55.0;
float temp = 0;
float humidity = 0;
DHTesp dht;
WiFiServer server(80);

void setup()
{
  // initialize LED digital pin as an output.
  pinMode(soundPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  Serial.begin(9600);
  digitalWrite(ledPin, HIGH);
  dht.setup(sensorPin, DHTesp::DHT11);
  
  WiFiManager wifiManager;
  wifiManager.autoConnect("HumidityAlarm");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //or use this for auto generated name ESP + ChipID
  server.begin();
  digitalWrite(ledPin, HIGH);
}

void alarm()
{
  if(alarmActivated){
    //toggle alarm on and off
    if(millis() / 1000 % 2 == 0){
      digitalWrite(4, HIGH);
      delay(3);
      digitalWrite(soundPin, LOW);
      delay(3);
    }else{
      digitalWrite(soundPin, LOW);
    }

    if((millis() / 250) % 2 == 0){
      digitalWrite(ledPin, HIGH);
    }else{
      digitalWrite(ledPin, LOW);
    }
  
  }else{
    digitalWrite(soundPin, LOW);
    digitalWrite(ledPin, LOW);
  }
}

unsigned long last_sample = 0;
void sample_humidity(){
  if(2100 < (millis() - last_sample)){
    humidity = dht.getHumidity();
    temp = dht.getTemperature();
    //Serial.print(dht.getStatusString());
    Serial.print("humidity: ");
    Serial.println(humidity, 1);
    Serial.print("temp: ");
    Serial.println(temp, 1);
    last_sample = millis();
  }
}

void handleNetwork(){

}

void loop()
{
  alarmActivated = humidity > alarmHumidity;
  sample_humidity();
  alarm();
}

