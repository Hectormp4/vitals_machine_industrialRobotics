#include <DIYablesWebApps.h>
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

#define MAX_BRIGHTNESS 255

const char WIFI_SSID[] = "YOUR NETWKK";
const char WIFI_PASSWORD[] = "NETWRK PASS";

// Create WebApp server and page instances
UnoR4ServerFactory serverFactory;
DIYablesWebAppServer webAppsServer(serverFactory, 80, 81);
DIYablesHomePage homePage;
DIYablesWebPlotterPage webPlotterPage;

// Timing variables
unsigned long lastDataTime = 0;
const unsigned long DATA_INTERVAL = 100;  // Send data every 100ms

// Sensor variable
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
uint16_t irBuffer[100];
uint16_t redBuffer[100];
#else
uint32_t irBuffer[100];
uint32_t redBuffer[100];
#endif

int32_t bufferLength = 100;
int32_t spo2 = 0;
int8_t validSPO2 = 0;
int32_t heartRate = 0;
int8_t validHeartRate = 0;

byte pulseLED = 11;
byte readLED = 13;

// Sample collection 
byte sampleIndex = 0;
bool initialSamplesCollected = false;
unsigned long lastSampleTime = 0;
const unsigned long SAMPLE_INTERVAL = 40; // 25 samples per second = 40ms per sample

void setup() {
  Serial.begin(9600);
  delay(1000);

  pinMode(pulseLED, OUTPUT);
  pinMode(readLED, OUTPUT);

  // Initialize MAX30105 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }

  Serial.println(F("Attach sensor to finger with rubber band. Press any key to start conversion"));
  while (Serial.available() == 0);
  Serial.read();

  // Configure sensor
  byte ledBrightness = 60;
  byte sampleAverage = 4;
  byte ledMode = 2;
  byte sampleRate = 100;
  int pulseWidth = 411;
  int adcRange = 4096;

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  
  Serial.println("DIYables WebApp - Web Plotter Example");

  // Add home and web plotter pages
  webAppsServer.addApp(&homePage);
  webAppsServer.addApp(&webPlotterPage);
  webAppsServer.setNotFoundPage(DIYablesNotFoundPage());

  // Configure the plotter with data series names
  webPlotterPage.setPlotTitle("Heart Rate (bpm) | SpO2 (%) | IR Signal");
  webPlotterPage.setAxisLabels("Time (s)", "Values");
  webPlotterPage.enableAutoScale(true);  // Auto-scale to fit all values
  webPlotterPage.setMaxSamples(50);
  // Start the WebApp server
  if (!webAppsServer.begin(WIFI_SSID, WIFI_PASSWORD)) {
    while (1) {
      Serial.println("Failed to start WebApp server!");
      delay(1000);
    }
  }

  // Set callback
  webPlotterPage.onPlotterDataRequest([]() {
    if (initialSamplesCollected) {
      if (validHeartRate) {
        Serial.print(heartRate);
      } else {
        Serial.print(0);
      }
      Serial.print(" ");
      if (validSPO2) {
        Serial.print(spo2);
      } else {
        Serial.print(0);
      }
      Serial.print(" ");
      
      // Send scaled IR signal for reference (divide by 1000 for better scale)
      Serial.println(irBuffer[sampleIndex > 0 ? sampleIndex - 1 : 0] / 1000);
    } else {
      Serial.println("0 0 0");
    }
  });

  Serial.println("\nWebPlotter is ready!");
  Serial.println("Usage Instructions:");
  Serial.println("1. Connect to the WiFi network");
  Serial.println("2. Open your web browser");
  Serial.println("3. Navigate to the Arduino's IP address");
  Serial.println("4. Click on 'Web Plotter' to view real-time data");
  Serial.println("\nCollecting sensor data...");
}

void loop() {
  webAppsServer.loop();

  // Non-blocking sensor check
  particleSensor.check();

  // Collect samples non-blocking
  if (millis() - lastSampleTime >= SAMPLE_INTERVAL) {
    lastSampleTime = millis();
    collectSample();
  }
}

void collectSample() {
  if (particleSensor.available()) {
    digitalWrite(readLED, !digitalRead(readLED));

    // Store sample
    redBuffer[sampleIndex] = particleSensor.getRed();
    irBuffer[sampleIndex] = particleSensor.getIR();
    particleSensor.nextSample();

    sampleIndex++;

    // After collecting 100 samples, calculate HR and SpO2
    if (sampleIndex >= 100) {
      sampleIndex = 0;
      initialSamplesCollected = true;

      // Calculate heart rate and SpO2
      maxim_heart_rate_and_oxygen_saturation (irBuffer, bufferLength, redBuffer,  &spo2, &validSPO2, &heartRate, &validHeartRate);

      // Print diagnostics (but not in plotter format - use different format)
      Serial.print(F("HR="));
      Serial.print(heartRate, DEC);
      Serial.print(F(", HRvalid="));
      Serial.print(validHeartRate, DEC);
      Serial.print(F(", SPO2="));
      Serial.print(spo2, DEC);
      Serial.print(F(", SPO2Valid="));
      Serial.println(validSPO2, DEC);

      // Optional: pulse LED based on heart rate validity
      if (validHeartRate) {
        analogWrite(pulseLED, MAX_BRIGHTNESS);
        delay(10);
        analogWrite(pulseLED, 0);
      }
    }
  }
}

