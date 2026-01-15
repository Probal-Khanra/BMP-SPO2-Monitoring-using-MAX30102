#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// OLED Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


MAX30105 particleSensor;


// Pin definitions
#define TTP223_PIN 27  // Touch sensor pin (change as needed)


// Mode definitions
enum Mode {
 MODE_BPM,
 MODE_SPO2
};


Mode currentMode = MODE_BPM;


// BPM variables
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;


// SpO2 variables
#define BUFFER_SIZE 100
uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;
int bufferIndex = 0;


// Proximity detection
bool fingerDetected = false;
unsigned long lastFingerTime = 0;


// Touch sensor debounce
unsigned long lastTouchTime = 0;
const unsigned long TOUCH_DEBOUNCE = 500;


void setup() {
 Serial.begin(115200);
 Serial.println("MAX30102 BPM/SpO2 Mode Switcher");


 pinMode(TTP223_PIN, INPUT);


 // Initialize OLED
 if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
   Serial.println("SSD1306 allocation failed");
   while(1);
 }
  display.clearDisplay();
 display.setTextSize(1);
 display.setTextColor(SSD1306_WHITE);
 display.setCursor(0, 0);
 display.println("Initializing...");
 display.display();
 delay(1000);


 // Initialize sensor
 if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
   Serial.println("MAX30102 not found!");
   display.clearDisplay();
   display.setCursor(0, 0);
   display.println("Sensor Error!");
   display.display();
   while (1);
 }


 byte ledBrightness = 60;
 byte sampleAverage = 4;
 byte ledMode = 2;
 byte sampleRate = 100;
 int pulseWidth = 411;
 int adcRange = 4096;


 particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  Serial.println("System ready!");
 Serial.println("Touch sensor to switch modes");
  displayMode();
}


void loop() {
 // Check for mode switch (touch sensor with debounce)
 bool touchState = digitalRead(TTP223_PIN);
  if (touchState == HIGH && (millis() - lastTouchTime > TOUCH_DEBOUNCE)) {
   lastTouchTime = millis();
   switchMode();
   delay(100);
 }


 // Check for finger proximity
 long irValue = particleSensor.getIR();
  if (irValue > 50000) { // Finger detected threshold
   if (!fingerDetected) {
     fingerDetected = true;
     Serial.println("Finger detected! Measuring...");
     updateDisplay("Finger detected!", "", "");
     delay(500);
   }
   lastFingerTime = millis();
  
   // Measure based on current mode
   switch (currentMode) {
     case MODE_BPM:
       measureBPM(irValue);
       break;
     case MODE_SPO2:
       measureSpO2();
       break;
   }
 } else {
   // No finger detected
   if (fingerDetected && (millis() - lastFingerTime > 1000)) {
     fingerDetected = false;
     Serial.println("Finger removed. Place finger to measure.");
     resetMeasurements();
     displayMode();
   } else if (!fingerDetected) {
     displayMode();
   }
 }


 delay(10);
}


void switchMode() {
 // Reset measurements
 resetMeasurements();
  // Switch mode
 currentMode = (currentMode == MODE_BPM) ? MODE_SPO2 : MODE_BPM;
  Serial.print("Switched to ");
 Serial.println((currentMode == MODE_BPM) ? "BPM Mode" : "SpO2 Mode");
  displayMode();
}


void displayMode() {
 display.clearDisplay();
 display.setTextSize(2);
 display.setCursor(0, 0);
  if (currentMode == MODE_BPM) {
   display.println("BPM MODE");
 } else {
   display.println("SpO2 MODE");
 }
  display.setTextSize(1);
 display.setCursor(0, 25);
 display.println("Place finger");
 display.println("on sensor");
  display.setCursor(0, 55);
 display.println("Touch to switch");
  display.display();
}


void updateDisplay(String line1, String line2, String line3) {
 display.clearDisplay();
 display.setTextSize(1);
 display.setCursor(0, 0);
 display.println(line1);
 display.setCursor(0, 20);
 display.println(line2);
 display.setCursor(0, 40);
 display.println(line3);
 display.display();
}


void measureBPM(long irValue) {
 if (checkForBeat(irValue)) {
   long delta = millis() - lastBeat;
   lastBeat = millis();


   beatsPerMinute = 60 / (delta / 1000.0);


   if (beatsPerMinute < 255 && beatsPerMinute > 20) {
     rates[rateSpot++] = (byte)beatsPerMinute;
     rateSpot %= RATE_SIZE;


     beatAvg = 0;
     for (byte x = 0; x < RATE_SIZE; x++) {
       beatAvg += rates[x];
     }
     beatAvg /= RATE_SIZE;


     Serial.print("BPM: ");
     Serial.print(beatsPerMinute);
     Serial.print(" | Avg BPM: ");
     Serial.println(beatAvg);
    
     // Update OLED
     display.clearDisplay();
     display.setTextSize(1);
     display.setCursor(0, 0);
     display.println("HEART RATE");
    
     display.setTextSize(3);
     display.setCursor(10, 20);
     display.print(beatAvg);
    
     display.setTextSize(1);
     display.setCursor(90, 30);
     display.println("BPM");
    
     display.setCursor(0, 55);
     display.print("Current: ");
     display.print((int)beatsPerMinute);
    
     display.display();
   }
 }
}


void measureSpO2() {
 // Read from sensor
 long irValue = particleSensor.getIR();
 redBuffer[bufferIndex] = particleSensor.getRed();
 irBuffer[bufferIndex] = particleSensor.getIR();
  // Calculate heart rate using same method as BPM mode
 if (checkForBeat(irValue)) {
   long delta = millis() - lastBeat;
   lastBeat = millis();


   beatsPerMinute = 60 / (delta / 1000.0);


   if (beatsPerMinute < 255 && beatsPerMinute > 20) {
     rates[rateSpot++] = (byte)beatsPerMinute;
     rateSpot %= RATE_SIZE;


     beatAvg = 0;
     for (byte x = 0; x < RATE_SIZE; x++) {
       beatAvg += rates[x];
     }
     beatAvg /= RATE_SIZE;
   }
 }
  bufferIndex++;
  // Show progress every 25 samples
 if (bufferIndex % 25 == 0) {
   Serial.print("Collecting samples: ");
   Serial.print(bufferIndex);
   Serial.print("/");
   Serial.println(BUFFER_SIZE);
  
   // Update OLED with progress
   display.clearDisplay();
   display.setTextSize(1);
   display.setCursor(0, 0);
   display.println("SpO2 Measuring");
  
   display.setTextSize(2);
   display.setCursor(20, 25);
   display.print(bufferIndex);
   display.print("/");
   display.println(BUFFER_SIZE);
  
   display.setTextSize(1);
   display.setCursor(0, 50);
   display.println("Keep finger still");
   display.display();
 }
  // When buffer is full, calculate SpO2
 if (bufferIndex >= BUFFER_SIZE) {
   bufferIndex = 0;
  
   // Calculate SpO2 using Maxim algorithm
   maxim_heart_rate_and_oxygen_saturation(irBuffer, BUFFER_SIZE, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  
   // Display results
   Serial.println("\n--- SpO2 Measurement ---");
  
   display.clearDisplay();
   display.setTextSize(1);
   display.setCursor(0, 0);
   display.println("BLOOD OXYGEN");
  
   if (validSPO2 == 1 && spo2 > 0 && spo2 <= 100) {
     Serial.print("SpO2: ");
     Serial.print(spo2);
     Serial.println("%");
    
     display.setTextSize(3);
     display.setCursor(25, 20);
     display.print(spo2);
     display.setTextSize(1);
     display.setCursor(90, 30);
     display.println("%");
    
   } else {
     Serial.println("SpO2: Calculating...");
     display.setTextSize(1);
     display.setCursor(0, 25);
     display.println("Calculating...");
     display.println("Keep finger still");
   }
  
   // Use the averaged BPM from beat detection instead of algorithm's heart rate
   if (beatAvg > 0) {
     Serial.print("Heart Rate: ");
     Serial.print(beatAvg);
     Serial.println(" BPM (avg)");
    
     display.setTextSize(1);
     display.setCursor(0, 55);
     display.print("HR: ");
     display.print(beatAvg);
     display.println(" BPM");
   }
  
   Serial.println("------------------------\n");
   display.display();
 }
}


void resetMeasurements() {
 // Reset BPM variables
 for (byte x = 0; x < RATE_SIZE; x++) {
   rates[x] = 0;
 }
 rateSpot = 0;
 lastBeat = 0;
 beatsPerMinute = 0;
 beatAvg = 0;
  // Reset SpO2 variables
 bufferIndex = 0;
 spo2 = 0;
 validSPO2 = 0;
 heartRate = 0;
 validHeartRate = 0;
  // Clear buffers
 for (int i = 0; i < BUFFER_SIZE; i++) {
   irBuffer[i] = 0;
   redBuffer[i] = 0;
 }
}
