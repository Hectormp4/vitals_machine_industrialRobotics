#include <MAX30100.h>
#include <Wire.h>
#include "Protocentral_MAX30205.h"

const int GSRpin = A0; 
MAX30205 tempSensor; 


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin();

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
  Serial.println(" Voltage value");
  delay(500);
 }
