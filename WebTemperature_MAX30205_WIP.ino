#include <DIYablesWebApps.h>
#include <Wire.h>
#include "Protocentral_MAX30205.h"

MAX30205 tempSensor;

// WiFi credentials - UPDATE THESE WITH YOUR NETWORK
const char WIFI_SSID[] = "Hector network";
const char WIFI_PASSWORD[] = "Swiss.!.13245";

// Create web app instances
UnoR4ServerFactory serverFactory;
DIYablesWebAppServer server(serverFactory, 80, 81);  // HTTP port 80, WebSocket port 81
DIYablesHomePage homePage;
DIYablesWebTemperaturePage temperaturePage(-10, 50.0, "°C");  // Min: -10°C, Max: 50°C

// Variables for tracking updates
float temperature = 0.0;
unsigned long lastUpdate = 0;

// Forward declaration of callback function
void onTemperatureValueRequested();

void setup() {
  Wire.begin();
  Serial.begin(9600);
  Serial.println("Starting Web Temperature Server...");
  
  // Add web apps to server
  server.addApp(&homePage);
  server.addApp(&temperaturePage);
  
  // Set 404 Not Found page (optional - for better user experience)
  server.setNotFoundPage(DIYablesNotFoundPage());
  
  // Set up temperature callback for value requests
  temperaturePage.onTemperatureValueRequested(onTemperatureValueRequested);
  
  // Start the server
  server.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Scan for temperature sensor every 30 sec until found
  while(!tempSensor.scanAvailableSensors()) {
    Serial.println("Couldn't find the temperature sensor, please connect the sensor.");
    delay(30000);
  }
  
  tempSensor.begin();   // set continuous mode, active mode
  Serial.println("Temperature sensor initialized successfully!");
}

void loop() {
  server.loop();
  
  // Send temperature update every 2 seconds
  if (millis() - lastUpdate >= 2000) {
    temperature = tempSensor.getTemperature();
    temperaturePage.sendTemperature(temperature);
    
    // Print temperature to Serial Monitor
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");
    
    lastUpdate = millis();
  }
  
  delay(10);  // Small delay for stability
}

/*
 * Callback function called when web interface requests temperature value
 * Send current temperature value to web interface
 */
void onTemperatureValueRequested() {
  Serial.println("Temperature value requested from web interface");
  // Send current temperature value (config is automatically sent by the library)
  temperaturePage.sendTemperature(temperature);
}
