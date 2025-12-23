/*
 * Real-Time Climate Monitor
 * Hardware: Arduino Uno, SCD30, SH1106 OLED, DS3231 RTC, SD Card
 * Logs CO2, Temperature, Humidity every 10 seconds
 */

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <Adafruit_SCD30.h>
#include <U8g2lib.h>

// Pin Definitions
#define SD_CS_PIN 10
#define LED_SD_STATUS 2
#define LED_LIGHTS 3
#define LIGHT_SENSOR_PIN A0

// Constants
#define LOG_INTERVAL 10000  // 10 seconds in milliseconds
#define LIGHT_THRESHOLD 950
#define I2C_CLOCK 100000    // 100kHz for reliability
#define SERIAL_BAUD 115200

// Global Objects
Adafruit_SCD30 scd30;
RTC_DS3231 rtc;
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

// State Variables
unsigned long lastLogTime = 0;
bool sdAvailable = false;
bool lightsOn = false;
String filename = "DATA.CSV";

// Function Prototypes
void initializeHardware();
void recoverI2C();
bool initializeSD();
void logData();
void updateDisplay(float co2, float temp, float humidity);
void checkLightLevel();
void processSerialCommands();
void blinkLED(uint8_t pin, uint8_t times);

void setup() {
  // Initialize Serial
  Serial.begin(SERIAL_BAUD);
  while (!Serial && millis() < 3000); // Wait up to 3s for serial
  
  Serial.println(F("\n=== Climate Monitor Starting ==="));
  
  // Initialize pins
  pinMode(LED_SD_STATUS, OUTPUT);
  pinMode(LED_LIGHTS, OUTPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  
  // Flash LEDs to show boot
  blinkLED(LED_SD_STATUS, 2);
  blinkLED(LED_LIGHTS, 2);
  
  // Initialize hardware
  initializeHardware();
  
  Serial.println(F("Setup complete. Logging every 10 seconds."));
  Serial.println(F("\nCommands: AUTOSET, SETTIME, CAL, SDCHECK, GETTIME\n"));
}

void loop() {
  unsigned long currentTime = millis();
  
  // Log data every 10 seconds
  if (currentTime - lastLogTime >= LOG_INTERVAL) {
    lastLogTime = currentTime;
    logData();
  }
  
  // Check light level (non-blocking)
  checkLightLevel();
  
  // Process serial commands (non-blocking)
  processSerialCommands();
  
  // Small delay to prevent CPU spinning
  delay(10);
}

void initializeHardware() {
  // Initialize I2C with safe clock speed
  Wire.begin();
  Wire.setClock(I2C_CLOCK);
  
  // Initialize RTC
  Serial.print(F("RTC: "));
  if (!rtc.begin()) {
    Serial.println(F("FAILED!"));
    blinkLED(LED_SD_STATUS, 5);
  } else {
    Serial.println(F("OK"));
    if (rtc.lostPower()) {
      Serial.println(F("RTC lost power, setting to compile time"));
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }
  
  // Initialize SCD30
  Serial.print(F("SCD30: "));
  recoverI2C();
  if (!scd30.begin()) {
    Serial.println(F("FAILED!"));
    blinkLED(LED_SD_STATUS, 5);
  } else {
    Serial.println(F("OK"));
    Serial.print(F("  Firmware: "));
    Serial.print(scd30.firmwareVersion(), HEX);
    Serial.println();
    
    // Configure SCD30 for efficiency
    scd30.setMeasurementInterval(2); // Measure every 2 seconds (minimum)
  }
  
  // Initialize Display
  Serial.print(F("Display: "));
  recoverI2C();
  if (!display.begin()) {
    Serial.println(F("FAILED!"));
  } else {
    Serial.println(F("OK"));
    display.setFont(u8g2_font_6x10_tr);
    display.clearBuffer();
    display.drawStr(0, 10, "Climate Monitor");
    display.drawStr(0, 25, "Initializing...");
    display.sendBuffer();
  }
  
  // Initialize SD Card
  sdAvailable = initializeSD();
}

bool initializeSD() {
  Serial.print(F("SD Card: "));
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("FAILED!"));
    digitalWrite(LED_SD_STATUS, LOW);
    return false;
  }
  
  Serial.println(F("OK"));
  
  // Create header if file doesn't exist
  if (!SD.exists(filename.c_str())) {
    File dataFile = SD.open(filename.c_str(), FILE_WRITE);
    if (dataFile) {
      dataFile.println(F("Timestamp,CO2(ppm),Temp(C),Humidity(%),Light,Lights"));
      dataFile.close();
      Serial.println(F("Created new CSV file"));
    }
  }
  
  digitalWrite(LED_SD_STATUS, HIGH);
  return true;
}

void logData() {
  // Check if SCD30 has data ready
  if (!scd30.dataReady()) {
    Serial.println(F("No data ready"));
    return;
  }
  
  // Read sensor data
  if (!scd30.read()) {
    Serial.println(F("Read failed"));
    recoverI2C();
    return;
  }
  
  float co2 = scd30.CO2;
  float temp = scd30.temperature;
  float humidity = scd30.relative_humidity;
  int lightLevel = analogRead(LIGHT_SENSOR_PIN);
  
  // Get timestamp from RTC
  DateTime now = rtc.now();
  
  // Format timestamp
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  
  // Print to Serial
  Serial.print(timestamp);
  Serial.print(F(" | CO2: "));
  Serial.print(co2, 0);
  Serial.print(F(" ppm | Temp: "));
  Serial.print(temp, 1);
  Serial.print(F(" C | Humidity: "));
  Serial.print(humidity, 1);
  Serial.print(F(" % | Light: "));
  Serial.print(lightLevel);
  Serial.print(F(" | Lights: "));
  Serial.println(lightsOn ? F("ON") : F("OFF"));
  
  // Log to SD card
  if (sdAvailable) {
    File dataFile = SD.open(filename.c_str(), FILE_WRITE);
    if (dataFile) {
      dataFile.print(timestamp);
      dataFile.print(',');
      dataFile.print(co2, 1);
      dataFile.print(',');
      dataFile.print(temp, 1);
      dataFile.print(',');
      dataFile.print(humidity, 1);
      dataFile.print(',');
      dataFile.print(lightLevel);
      dataFile.print(',');
      dataFile.println(lightsOn ? '1' : '0');
      dataFile.close();
      
      // Quick blink to show successful write
      digitalWrite(LED_SD_STATUS, LOW);
      delay(50);
      digitalWrite(LED_SD_STATUS, HIGH);
    } else {
      Serial.println(F("SD write failed"));
      sdAvailable = false;
      digitalWrite(LED_SD_STATUS, LOW);
    }
  }
  
  // Update display
  updateDisplay(co2, temp, humidity);
}

void updateDisplay(float co2, float temp, float humidity) {
  DateTime now = rtc.now();
  
  display.clearBuffer();
  
  // Time
  char timeStr[9];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
           now.hour(), now.minute(), now.second());
  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(0, 10, timeStr);
  
  // Date
  char dateStr[11];
  snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", 
           now.year(), now.month(), now.day());
  display.drawStr(65, 10, dateStr);
  
  // CO2
  display.setFont(u8g2_font_9x15_tr);
  char co2Str[16];
  snprintf(co2Str, sizeof(co2Str), "CO2:%4d ppm", (int)co2);
  display.drawStr(0, 28, co2Str);
  
  // Temperature
  display.setFont(u8g2_font_7x13_tr);
  char tempStr[12];
  snprintf(tempStr, sizeof(tempStr), "T:%4.1f C", temp);
  display.drawStr(0, 44, tempStr);
  
  // Humidity
  char humStr[12];
  snprintf(humStr, sizeof(humStr), "H:%4.1f %%", humidity);
  display.drawStr(0, 58, humStr);
  
  // Status indicators
  if (sdAvailable) {
    display.drawStr(105, 58, "SD");
  }
  if (lightsOn) {
    display.drawCircle(120, 52, 3);
  }
  
  display.sendBuffer();
}

void checkLightLevel() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 1000) return; // Check every second
  lastCheck = millis();
  
  int lightLevel = analogRead(LIGHT_SENSOR_PIN);
  bool shouldBeLit = (lightLevel <= LIGHT_THRESHOLD);
  
  if (shouldBeLit != lightsOn) {
    lightsOn = shouldBeLit;
    digitalWrite(LED_LIGHTS, lightsOn ? HIGH : LOW);
  }
}

void processSerialCommands() {
  if (!Serial.available()) return;
  
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toUpperCase();
  
  if (cmd == "AUTOSET") {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println(F("RTC set to compile time"));
    
  } else if (cmd.startsWith("SETTIME")) {
    // Parse: SETTIME 2025 12 23 15 30 00
    int y, mo, d, h, mi, s;
    if (sscanf(cmd.c_str(), "SETTIME %d %d %d %d %d %d", &y, &mo, &d, &h, &mi, &s) == 6) {
      rtc.adjust(DateTime(y, mo, d, h, mi, s));
      Serial.println(F("RTC time set"));
    } else {
      Serial.println(F("Format: SETTIME YYYY MM DD HH MM SS"));
    }
    
  } else if (cmd.startsWith("CAL")) {
    uint16_t reference = 400;
    if (cmd.length() > 4) {
      reference = cmd.substring(4).toInt();
    }
    if (scd30.forceRecalibrationWithReference(reference)) {
      Serial.print(F("Calibrated to "));
      Serial.print(reference);
      Serial.println(F(" ppm"));
    } else {
      Serial.println(F("Calibration failed"));
    }
    
  } else if (cmd == "SDCHECK") {
    sdAvailable = initializeSD();
    
  } else if (cmd == "GETTIME") {
    DateTime now = rtc.now();
    char buf[25];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
    Serial.println(buf);
    
  } else {
    Serial.println(F("Unknown command"));
  }
}

void recoverI2C() {
  Wire.end();
  delay(100);
  Wire.begin();
  Wire.setClock(I2C_CLOCK);
  delay(100);
}

void blinkLED(uint8_t pin, uint8_t times) {
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(100);
    digitalWrite(pin, LOW);
    delay(100);
  }
}
