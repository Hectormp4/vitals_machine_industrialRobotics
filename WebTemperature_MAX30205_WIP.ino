#include <DIYablesWebApps.h>
#include <Wire.h>
#define MAX30205_I2C_Address 0x48
// WiFi credentials - UPDATE THESE WITH YOUR NETWORK
const char WIFI_SSID[] = "NETWORK_NAME";
const char WIFI_PASSWORD[] = "NETWORK_PASSWORD";
// Create web app instances
UnoR4ServerFactory serverFactory;
DIYablesWebAppServer server(serverFactory, 80, 81);  // HTTP port 80, WebSocket port 81
DIYablesHomePage homePage;
DIYablesWebTemperaturePage temperaturePage(-50.0, 80.0, "°C");  // Min: -10°C, Max: 50°C
// Temperature variables
unsigned long lastUpdate = 0;
static float temperature;
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
}
void loop() {
  server.loop();
      // Send temperature update every 2 seconds
  if (millis() - lastUpdate >= 2000) {
    temperature = readTemperature();
    temperaturePage.sendTemperature(temperature);
    // Print temperature to Serial Monitor
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");
    lastUpdate = millis();
  }
    
  delay(10);  // Small delay for stability
}
float readTemperature() {
  Wire.beginTransmission(MAX30205_I2C_Address); // Start communication with MAX30205
  Wire.write(0x00); // Point to the temperature register
  Wire.endTransmission(false); // Send the address and keep the connection active
  Wire.requestFrom(MAX30205_I2C_Address, 2); // Request 2 bytes of temperature data
  if (Wire.available() == 2) {
    uint8_t msb = Wire.read(); // Read the most significant byte
    uint8_t lsb = Wire.read(); // Read the least significant byte
    int16_t rawTemperature = (msb << 8) | lsb; // Combine the two bytes
    return rawTemperature * 0.00390625; // Convert to Celsius (0.0625°C per LSB)
  } else {
    return NAN; // Return NaN if data is not available
  }
}
/**
 * Callback function called when web interface requests temperature value
 * Send current temperature value to web interface
 */
void onTemperatureValueRequested() {
  Serial.println("Temperature value requested from web interface");
  // Send current temperature value (config is automatically sent by the library)
  temperaturePage.sendTemperature(temperature);
}

