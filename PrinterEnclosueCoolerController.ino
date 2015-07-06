/*********************************************************************
 * 
 * Printer Enclosure Cooler Controller
 * (C)Copyright 2015 - Michael Teeuw
 * 
 * http://michaelteeuw.nl
 * 
 * Licenced unther the The MIT License (MIT)
 * Please Check the LICENCE file.
 * 
*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <elapsedMillis.h>

#define DHTPIN 11           // The Arduino pin connected to the DHT22's DATA Pin   
#define FAN_POWER_PIN 10    // The Arduino pin connected to Mosfet  
#define FAN_PULSE_PIN 9     // The Arduino pin connected to the FAN's Sense wire (green)
#define FAN_PWM_PIN 3       // The Arduino pin connected to the FAN's Control wire (blue)

#define FAN_ON_TEMP 25      // The minimum temperature to turn on the Fan.
#define FAN_MIN_TEMP 30     // The start temperature for variable speed control.
#define FAN_MAX_TEMP 40     // The end temperature for variable speed control. 

#define FAN_PWM_MIN 64      // = 25% - The minimum speed for variable control.

#define SCREEN_WIDTH 128    // The Width of the OLED
#define SCREEN_HEIGHT 64    // The Height of the OLED

#define HISTORY_SIZE 126    // The Number of History Items
#define HISTORY_INTERVAL 30 // The number of seconds between every history item.


// Create all objects for hardware control.
Adafruit_SSD1306 display(4);
DHT dht(DHTPIN, DHT22);

// Create global variables.
elapsedMillis historyInterval;
int humidity = 0;
int rpm = 0;
byte fanSpeed = 0;
bool fanPowered = true;
float temperature = 0;

byte speedHistory[HISTORY_SIZE];


/*
 * Initialize Setup
 */
void setup()   {                

  // Initialize the screen.
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();   // clears the screen and buffer

  // Fill the history array with 0s.
  for (int i = 0; i < HISTORY_SIZE; i++) {
    speedHistory[i] = 0;
  }

  // Set the pinmodes for the used pins.
  pinMode(FAN_PULSE_PIN, INPUT_PULLUP);
  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(FAN_POWER_PIN, OUTPUT);

  // Set the start state of the mosfet.
  digitalWrite(FAN_POWER_PIN, fanPowered);
}


/*
 * Loop
 */
void loop() {

  // Read the current Temperature and Humidity
  humidity = (int) dht.readHumidity();
  temperature = (float) dht.readTemperature();

  // Measure and calculate the current RPM of the Fan.
  unsigned long pulseDuration = pulseIn(FAN_PULSE_PIN, LOW);
  rpm = 60 * 1000000 / pulseDuration / 4;

  // Calculate the desired fan speed depending on the current temperature.
  // The themperature and map inputs are multiplied with 100 because we 
  // want to take the decimals into account. The map function only takes
  // unsigned numbers.
  
  fanSpeed = constrain(map(temperature * 100, FAN_MIN_TEMP * 100, FAN_MAX_TEMP * 100, FAN_PWM_MIN, 255), 0, 255);
  analogWrite(FAN_PWM_PIN, fanSpeed);

  // Set the fan power (mosfet on/off) based on the current temperature.
 
  fanPowered = temperature >= FAN_ON_TEMP;
  digitalWrite(FAN_POWER_PIN, fanPowered);

  // Every loop check if it's time to add the current Fan speed to the history array.
  if (historyInterval >= HISTORY_INTERVAL*1000) {    
    addSpeedToHistory(constrain(fanSpeed,FAN_PWM_MIN, 255));
    historyInterval = 0;
  }

  // Update the screen with the new info.
  updateUI ();

  // Wait half a second before we loop again.
  delay(500);
}

void updateUI () {

  // Clear the display
  display.clearDisplay();

  // Set the desired text style.
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Print the Humidity
  display.setCursor(0,0);
  display.print("HUM: ");
  display.print(humidity);
  display.print("%");
  
  // Print the Fan
  display.setCursor(0,8);
  display.print("FAN: ");
  if (fanPowered) {
    if (fanSpeed > FAN_PWM_MIN) {
      display.print(map(fanSpeed, 0, 255, 0, 100));
      display.print("%");
    } else {
      display.print("MIN");
    }
  } else {
    display.print("OFF");
  }
  
  // Print the RPM
  display.setCursor(0,16);
  display.print("RPM: ");
  display.print((rpm == -1) ? "OFF" : String(rpm));
  
  // Print Temperature decimal right aligned usering a seperate function.
  printRight(String(decimalValue(temperature, 10)), 2, SCREEN_WIDTH, 0); 
  
  // Print Temperature integer right aligned usering a seperate function.
  printRight(String((int) temperature), 3, SCREEN_WIDTH - 12, 2); 

  // Draw the graph in a seperate function.
  drawHistoryGraph();

  display.display();
}

/*
 * Function to add a value to the history array.
 * Maybe there is an easier way to do this?
 */
void addSpeedToHistory(byte speed) {

  // Create a new temporary array.
  byte newSpeedHistory[HISTORY_SIZE];

  // Copy the values from the real tempArray with an offset of -1.
  for (int i = 1; i < HISTORY_SIZE; i++) {
    newSpeedHistory[i - 1] = speedHistory[i];
  }

  // Add a the new value to the temp array.
  newSpeedHistory[HISTORY_SIZE - 1] = speed;

  // Copy everything back into the real array.
  for (int i = 0; i < HISTORY_SIZE; i++) {
    speedHistory[i] = newSpeedHistory[i];
  }
}

/*
 * Retrieve the decimal part of a float.
 * The multiplier decides how many decimals you like.
 * 10 = 1 decimal.
 * 100 = 2 decimals.
 * 1000 = 3 decimals. ... etc
 */
int decimalValue(float value, int multiplier) {
  int integerValue = (int)(value);
  float decimalValue = value - integerValue;
  
  return decimalValue * multiplier;
}

/* 
 *  Function to print a text to the display, right aligned to the coordinates.
 */
void printRight(String text, byte size, byte x, byte y) {
  byte stringLength = text.length();
  
  int textSize = size * 5 * stringLength;
  int spaceSize = size * (stringLength - 1);
  
  display.setTextSize(size);
  display.setCursor(x - textSize - spaceSize, y);
  display.print(text); 
}

/*
 * Function to draw the history graph.
 */
void drawHistoryGraph() {
  byte yOffset = 27;
  byte height = 37;
  
  display.drawRect(0 , yOffset, SCREEN_WIDTH, height, WHITE);
 
  for (int i = 0; i < HISTORY_SIZE; i++) {
    
    byte mappedValue = constrain(map(speedHistory[i], 0, 255, 0, height - 2), 0, height - 2);
    
    byte x = i + 1;
    byte startY = yOffset + height - 1;
    byte endY = startY - mappedValue;
    
    display.drawLine(x, startY, x, endY, WHITE);
  }
}


