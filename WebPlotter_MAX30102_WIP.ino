#include <DIYablesWebApps.h>
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 particleSensor;

#define MAX_BRIGHTNESS 255

const char WIFI_SSID[] = "Nacho WiFi";
const char WIFI_PASSWORD[] = "WERaw3s0me!";

// Create WebApp server and page instances
UnoR4ServerFactory serverFactory;
DIYablesWebAppServer webAppsServer(serverFactory, 80, 81);
DIYablesHomePage homePage;
DIYablesWebPlotterPage webPlotterPage;

// Timing variables
unsigned long lastDataTime = 0;
const unsigned long DATA_INTERVAL = 1000;  // Send data every 1000ms (1 Hz)

// Sensor variables
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
const unsigned long SAMPLE_INTERVAL = 40;  // 25 samples per second = 40ms per sample

void setup() {
  Serial.begin(9600);
  delay(1000);

  pinMode(pulseLED, OUTPUT);
  pinMode(readLED, OUTPUT);

  Serial.println("DIYables WebApp - SpO2 & BPM Monitor");

  // Initialize MAX30105 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }

  Serial.println(F("\nAttach sensor to finger with rubber band."));
  Serial.println(F("Press any key to start monitoring..."));
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

  // Add home and web plotter pages
  webAppsServer.addApp(&homePage);
  webAppsServer.addApp(&webPlotterPage);
  webAppsServer.setNotFoundPage(DIYablesNotFoundPage());

  // Configure the plotter
  webPlotterPage.setPlotTitle("SpO2 & Heart Rate Monitor");
  webPlotterPage.setAxisLabels("Time (s)", "Values");
  webPlotterPage.enableAutoScale(true);  // Auto-scale to fit all values
  webPlotterPage.setMaxSamples(60);

  // Start the WebApp server
  if (!webAppsServer.begin(WIFI_SSID, WIFI_PASSWORD)) {
    while (1) {
      Serial.println("Failed to start WebApp server!");
      delay(1000);
    }
  }

  // Set callback for web data requests
  webPlotterPage.onPlotterDataRequest([]() {
    sendVitalSigns();
  });

  Serial.println("\nWebPlotter is ready!");
  Serial.println("Usage Instructions:");
  Serial.println("1. Connect to the WiFi network");
  Serial.println("2. Open your web browser");
  Serial.println("3. Navigate to the Arduino's IP address");
  Serial.println("4. Click on 'Web Plotter' to view real-time vital signs");
  Serial.println("\nInitializing sensor...");
  Serial.println("Gathering baseline data samples...");
}

void loop() {
  // Handle web server and WebSocket connections
  webAppsServer.loop();

  // Non-blocking sensor check
  particleSensor.check();

  // Collect samples at regular intervals
  if (millis() - lastSampleTime >= SAMPLE_INTERVAL) {
    lastSampleTime = millis();
    collectSample();
  }

  // Send data to plotter at regular intervals
  if (initialSamplesCollected && millis() - lastDataTime >= DATA_INTERVAL) {
    lastDataTime = millis();
    sendVitalSigns();
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
      
      if (!initialSamplesCollected) {
        Serial.println("............");
        Serial.println("Sensor ready!");
        Serial.println("\nMonitoring vital signs...");
        initialSamplesCollected = true;
      }

      // Calculate heart rate and SpO2
      maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

      // Optional: pulse LED based on heart rate validity
      if (validHeartRate) {
        analogWrite(pulseLED, MAX_BRIGHTNESS);
        delay(10);
        analogWrite(pulseLED, 0);
      }
    }
  }
}

void sendVitalSigns() {
  // Use the calculated values from the sensor
  int displayHeartRate = validHeartRate ? heartRate : 0;
  int displaySpo2 = validSPO2 ? spo2 : 0;

  // Send data to web plotter (only HR and SpO2)
  webPlotterPage.sendPlotData(displaySpo2, displayHeartRate);

  // Print to Serial Monitor in the format: HR=XX, SPO2=XX
  Serial.print(F("HR="));
  Serial.print(displayHeartRate, DEC);
  Serial.print(F(", SPO2="));
  Serial.println(displaySpo2, DEC);
}
