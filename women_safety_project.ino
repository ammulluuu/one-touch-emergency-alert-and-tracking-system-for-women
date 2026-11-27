#include <Wire.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// Reconfigured LCD connection based on your breakout layout
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

// Arduino RX (2) <- GSM TX | Arduino TX (3) -> GSM RX
SoftwareSerial sharedSerial(2, 3);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Component Pin Definitions
const int TOUCH_SOS_PIN = 4;
const int SAFETY_BUTTON_PIN = 5;
const int BUZZER_PIN = 7; // CHANGED TO PIN 7 AS REQUESTED

// Configurable Constants
const unsigned long SAFETY_TIMEOUT = 5000;

// Accident Condition Limits (Based on X and Y axis acceleration)
const int ACCIDENT_THRESHOLD_MAX = 15;  // Adjust based on your testing
const int ACCIDENT_THRESHOLD_MIN = -15; // Adjust based on your testing

// FIXED COORDINATES: Replace these values with your custom static location
const String FIXED_LATITUDE = "17.5585223";   
const String FIXED_LONGITUDE = "78.4537806";
  

const String googleMapsUrl = "https://google.com" + FIXED_LATITUDE + "," + FIXED_LONGITUDE;

// Add your 3 registered mobile numbers here
const String registeredNumbers[1] = {  "+918309193202" };

// Global Tracking Variables
unsigned long lastButtonPressTime = 0;

void setup(void) {
  Serial.begin(9600);
  sharedSerial.begin(9600); // SIM900A operates at 9600 baud
  
  lcd.begin(16, 2);
  pinMode(TOUCH_SOS_PIN, INPUT);
  pinMode(SAFETY_BUTTON_PIN, INPUT_PULLUP);
  
  // REVERSE POLARITY: Set pin HIGH initially so the buzzer stays OFF at boot
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH); 

  /* Initialise the sensor */
  if (!accel.begin()) {
    Serial.println("Not Working");
    lcd.clear();
    lcd.print("ADXL345 Error!");
    while (1);
  }

  /* Set the range to whatever is appropriate for your project */
  accel.setRange(ADXL345_RANGE_16_G);

  // Warm-up delay for components
  delay(3000);
  lastButtonPressTime = millis();
  
  lcd.clear();
  lcd.print("   SECURFHER    ");
  lcd.setCursor(0,1);
  lcd.print("EMERGENCY ALERT ");
  delay(2000);
  lcd.clear();
  lcd.print("System Active");
  delay(1000);
}

void loop(void) {
  checkSensors();
}

// Function to monitor alerts and safety timers
void checkSensors() {
  // 1. Touch SOS Button Monitoring
  if (digitalRead(TOUCH_SOS_PIN) == HIGH) {
    triggerEmergency("SOS TRIPPED!");
  }

  // 2. Dead Man's Safety Switch Monitoring
  if (digitalRead(SAFETY_BUTTON_PIN) == LOW) {
    lastButtonPressTime = millis(); // Reset safety timeout window
    lcd.setCursor(0, 0);
    lcd.print("Safety Verified ");
    delay(50); // Simple debounce
  }

  // Check if safety interval has exceeded 5 seconds
  if (millis() - lastButtonPressTime > SAFETY_TIMEOUT) {
    triggerEmergency("TIMEOUT ALERT!");
  }

  // 3. ADXL345 Crash & Fall Monitoring (Integrated from your custom layout)
  sensors_event_t event;
  accel.getEvent(&event);
  int x_val = event.acceleration.x;
  int y_val = event.acceleration.y;

  // Print to Serial Monitor as requested
  Serial.print("X-axis: ");
  Serial.print(x_val);
  Serial.print("     ");
  Serial.print("Y-axis: ");
  Serial.println(y_val);

  // Threshold check to trigger accident mode on sudden impacts or tilts
  if (x_val > ACCIDENT_THRESHOLD_MAX || x_val < ACCIDENT_THRESHOLD_MIN ||
      y_val > ACCIDENT_THRESHOLD_MAX || y_val < ACCIDENT_THRESHOLD_MIN) {
    triggerEmergency("ACCIDENT DETECT!");
  }
  
  delay(50); // Small delay to avoid serial flooding, modified from 500ms to preserve system responsiveness
}

// Broadcast alerts to local hardware and transmit over shared serial TX
void triggerEmergency(String alertReason) {
  // REVERSE POLARITY: Drive LOW to turn the buzzer ON
  digitalWrite(BUZZER_PIN, LOW); 
  
  lcd.clear();
  lcd.print("EMERGENCY MODE!");
  lcd.setCursor(0, 1);
  lcd.print(alertReason);

  // Dispatch SMS to all three registered numbers via GSM
  for (int i = 0; i < 3; i++) {
    sendSMS(registeredNumbers[i], "ALERT: " + alertReason + " Track here: " + googleMapsUrl);
    delay(1000);
  }

  // Lock system into safety hazard state with a reversing alarm pulse
  while (true) {
    digitalWrite(BUZZER_PIN, LOW);  // Buzzer ON
    delay(300);
    digitalWrite(BUZZER_PIN, HIGH); // Buzzer OFF
    delay(300);
  }
}

// AT Command interface routine outputting strictly to SIM900A RX via Pin 3
void sendSMS(String number, String text) {
  sharedSerial.println("AT+CMGF=1"); // Set text message mode
  delay(300);
  sharedSerial.print("AT+CMGS=\"");
  sharedSerial.print(number);
  sharedSerial.println("\"");
  delay(300);
  sharedSerial.print(text);
  delay(300);
  sharedSerial.write(26); // Send Ctrl+Z terminator sequence to send message
  delay(4000); // Wait for network delivery processing
}