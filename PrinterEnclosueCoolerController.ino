/*********************************************************************
*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <elapsedMillis.h>

#define DHTPIN 11 
#define DHTTYPE DHT22 
#define OLED_RESET 4
#define FAN_POWER_PIN 10
#define FAN_PULSE_PIN 9
#define FAN_PMW_PIN 3

#define FAN_ON_TEMP 25
#define FAN_MIN_TEMP 30
#define FAN_MAX_TEMP 40

#define FAN_PMW_MIN 64 // = 25%


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define HISTORY_SIZE 126
#define HISTORY_INTERVAL 30

Adafruit_SSD1306 display(OLED_RESET);
DHT dht(DHTPIN, DHTTYPE);
elapsedMillis historyInterval;

int humidity = 0;
int rpm = 0;
byte fanSpeed = 0;
bool fanPowered = true;
float temperature = 0;

int tempHistory[HISTORY_SIZE];

void setup()   {                

  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  

  display.clearDisplay();   // clears the screen and buffer

  for (int i = 0; i < HISTORY_SIZE; i++) {
    tempHistory[i] = 0;
  }
  
  pinMode(FAN_PULSE_PIN, INPUT_PULLUP);
  pinMode(FAN_PMW_PIN, OUTPUT);

  pinMode(FAN_POWER_PIN, OUTPUT);
  digitalWrite(FAN_POWER_PIN, fanPowered);
}


void loop() {
  
  humidity = (int) dht.readHumidity();
  temperature = (float) dht.readTemperature();
  
  unsigned long pulseDuration = pulseIn(FAN_PULSE_PIN, LOW);
  rpm = 60 * 1000000 / pulseDuration / 4;
  
  fanSpeed = constrain(map(temperature * 100, FAN_MIN_TEMP * 100, FAN_MAX_TEMP * 100, FAN_PMW_MIN, 255), 0, 255);
  analogWrite(FAN_PMW_PIN, fanSpeed);
  
  fanPowered = temperature >= FAN_ON_TEMP;
  digitalWrite(FAN_POWER_PIN, fanPowered);
  
  
  if (historyInterval >= HISTORY_INTERVAL*1000) {    
    addTemperatureToHistory(temperature * 100);
    historyInterval = 0;
  }
  
  
 
  
  updateUI ();


  delay(500);
}

void updateUI () {
  display.clearDisplay();
  
  
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Humidity
  display.setCursor(0,0);
  display.print("HUM: ");
  display.print(humidity);
  display.print("%");
  
  // Fan
  display.setCursor(0,8);
  display.print("FAN: ");
  if (fanPowered) {
    if (fanSpeed > FAN_PMW_MIN) {
      display.print(map(fanSpeed, 0, 255, 0, 100));
      display.print("%");
    } else {
      display.print("MIN");
    }
  } else {
    display.print("OFF");
  }
  
  // RPM
  display.setCursor(0,16);
  display.print("RPM: ");
  display.print((rpm == -1) ? "OFF" : String(rpm));
  

  // Print Temperature decimal
  printRight(String(decimalValue(temperature, 10)), 2, SCREEN_WIDTH, 0); 
  
  // Print Temperature integer
  printRight(String((int) temperature), 3, SCREEN_WIDTH - 12, 2); 

 
  drawHistoryGraph();

  display.display();

}

void addTemperatureToHistory(byte temp) {
  byte newTempHistory[HISTORY_SIZE];
  
  for (int i = 1; i < HISTORY_SIZE; i++) {
    newTempHistory[i - 1] = tempHistory[i];
  }
  
  newTempHistory[HISTORY_SIZE - 1] = temp;
  
  for (int i = 0; i < HISTORY_SIZE; i++) {
    tempHistory[i] = newTempHistory[i];
  }
}

int decimalValue(float value, int multiplier) {
  int integerValue = (int)(value);
  float decimalValue = value - integerValue;
  
  
  return decimalValue * multiplier;
}

void printRight(String text, byte size, byte x, byte y) {
  byte stringLength = text.length();
  
  int textSize = size * 5 * stringLength;
  int spaceSize = size * (stringLength - 1);
  
  
  display.setTextSize(size);
  display.setCursor(x - textSize - spaceSize, y);
  display.print(text); 
}

void drawHistoryGraph() {
  byte yOffset = 27;
  byte height = 37;
  
  display.drawRect(0 , yOffset, SCREEN_WIDTH, height, WHITE);
 
  int min = minimumTemperature();
  int max = maximumTemperature();
 
  for (int i = 0; i < HISTORY_SIZE; i++) {
    
    byte mappedValue = constrain(map(tempHistory[i], min, max, 0, height - 2), 0, height - 2);
    
    byte x = i + 1;
    byte startY = yOffset + height - 1;
    byte endY = startY - mappedValue;
    
    display.drawLine(x, startY, x, endY, WHITE);
    
    //tempHistory[i] = random(0, 40);
  }
  
}

int minimumTemperature() {
  int min = maximumTemperature();
  for (int i = 0; i < HISTORY_SIZE; i++) {
    if (tempHistory[i] < min && tempHistory[i] != 0) {
      min = tempHistory[i];
    }
  }
  
  return min;
}

int maximumTemperature() {
  int max = tempHistory[0];
  for (int i = 1; i < HISTORY_SIZE; i++) {
    if (tempHistory[i] > max) {
      max = tempHistory[i];
    }
  }
  
  return max;
}


