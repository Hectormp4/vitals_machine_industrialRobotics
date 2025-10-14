#include <Wire.h>
#include "Protocentral_MAX30205.h"
#include "MAX30100_PulseOximeter.h"
#define REPORTING_PERIOD_MS 1000
const int GSRpin = A0;

MAX30205 tempSensor; 
PulseOximeter pox;

uint32_t tsLastReport = 0;

void onBeatDetected(){
  Serial.println("Beat!!");
}


void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);
  Serial.begin(115200);  

  //MAX30100 scans 
  Serial.println("Initializing pulse oximeter..");
  if (!pox.begin()){
    Serial.println("FAILED");
    for(;;);
    }else{
      Serial.println("SUCCESS");
    }
    pox.setOnBeatDetectedCallback(onBeatDetection)
  }

  //MAX30205 scans
  while(!tempSensor.scanAvailableSensors()){
    Serial.println("No sensor input was found, please retry.");
    delay(30000); //scans every 30 seconds
  }

  tempSensor.begin(); //makes the scanning constant 
}

void loop() {
  // MAX30205 sensor
  float temp = tempSensor.getTemperature();
  Serial.print(temp ,2);
  Serial.println(" Celsius"); 
  delay(500); 


  //GSR sensor
  int sensorValue = analogRead(GSRpin);
  float voltage = sensorValue * (5/1023);
  Serial.print(voltage);
  Serial.println(" : Voltage value");
  delay(500);
 }
