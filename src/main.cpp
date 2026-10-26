/*
  SALT SPARK — Seawater-Powered Water Quality Monitoring Buoy
  Starter firmware for ESP32-S3 + analog pH/EC sensors + DS18B20 + SX1262 LoRa.

  IMPORTANT:
  - Confirm every GPIO against your exact ESP32-S3 board and wiring.
  - Sensor boards vary. Calibrate pH and EC with known buffer/standard solutions.
  - The EC conversion below is intentionally a placeholder until your sensor module
    and calibration standard are known.
  - This is a prototype starter, not validated marine instrumentation.
*/

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------- PIN CONFIGURATION: CHANGE TO MATCH YOUR WIRING ----------
static const int PIN_PH_ANALOG = 1;       // Analog output from pH interface board
static const int PIN_EC_ANALOG = 2;       // Analog output from EC/salinity interface board
static const int PIN_DS18B20   = 4;       // DS18B20 data pin; use a 4.7k pull-up to 3.3V

// SX1262 SPI pins: edit for your board/wiring.
static const int PIN_LORA_SCK  = 12;
static const int PIN_LORA_MISO = 13;
static const int PIN_LORA_MOSI = 11;
static const int PIN_LORA_NSS  = 10;
static const int PIN_LORA_DIO1 = 9;
static const int PIN_LORA_RST  = 8;
static const int PIN_LORA_BUSY = 7;

// India LoRa band is generally 865–867 MHz; verify local rules and module variant.
static const float LORA_FREQUENCY_MHZ = 866.5;

// ---------- ADC CONFIGURATION ----------
static const float ADC_REFERENCE_V = 3.3f;
static const int ADC_MAX_COUNTS = 4095;

// ---------- pH CALIBRATION ----------
// Placeholder two-point linear calibration: replace after measuring pH 7 and pH 4/10 buffers.
// Formula: pH = PH_SLOPE * voltage + PH_OFFSET
static const float PH_SLOPE  = -5.70f;
static const float PH_OFFSET = 21.34f;

// ---------- EC CALIBRATION ----------
// Do NOT treat this as a real salinity reading.
// Set these only after calibration for your specific EC module/probe.
// Example formula: EC_mS_cm = EC_SLOPE * voltage + EC_OFFSET
static const float EC_SLOPE  = 1.0f;
static const float EC_OFFSET = 0.0f;

static const unsigned long SAMPLE_INTERVAL_MS = 30000;
static unsigned long lastSampleMs = 0;

OneWire oneWire(PIN_DS18B20);
DallasTemperature tempSensor(&oneWire);

// SX1262 constructor order: NSS, DIO1, RESET, BUSY
SX1262 radio = new Module(PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RST, PIN_LORA_BUSY);

float readAnalogVoltage(int pin) {
  // Average multiple ADC samples to reduce noise.
  uint32_t total = 0;
  const int samples = 20;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(3);
  }
  float averageCounts = total / (float)samples;
  return (averageCounts / ADC_MAX_COUNTS) * ADC_REFERENCE_V;
}

float readPH(float voltage) {
  return PH_SLOPE * voltage + PH_OFFSET;
}

float readEC_mS_cm(float voltage) {
  return EC_SLOPE * voltage + EC_OFFSET; // placeholder; calibrate for your probe
}

void sendLoRaPacket(const String& payload) {
  Serial.print("LoRa TX: ");
  Serial.println(payload);

  int state = radio.transmit(payload);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("LoRa transmit: success");
  } else {
    Serial.print("LoRa transmit failed, code: ");
    Serial.println(state);
    Serial.println("Check frequency, antenna, wiring, power and SX1262 variant.");
  }
}

void sampleAndTransmit() {
  float phVoltage = readAnalogVoltage(PIN_PH_ANALOG);
  float ecVoltage = readAnalogVoltage(PIN_EC_ANALOG);

  tempSensor.requestTemperatures();
  float temperatureC = tempSensor.getTempCByIndex(0);
  bool tempValid = (temperatureC != DEVICE_DISCONNECTED_C && temperatureC > -55.0f
                    && temperatureC < 125.0f);

  float phValue = readPH(phVoltage);
  float ecValue = readEC_mS_cm(ecVoltage);

  // Simple JSON payload for a receiver/dashboard to parse.
  // EC is marked as uncalibrated until you set the calibration constants.
  String payload = "{";
  payload += "\"device\":\"SALT_SPARK\",";
  payload += "\"uptime_s\":" + String(millis() / 1000) + ",";
  payload += "\"ph_voltage_v\":" + String(phVoltage, 3) + ",";
  payload += "\"ph_estimate\":" + String(phValue, 2) + ",";
  payload += "\"ec_voltage_v\":" + String(ecVoltage, 3) + ",";
  payload += "\"ec_estimate_mS_cm_uncalibrated\":" + String(ecValue, 2) + ",";
  payload += "\"temperature_c\":";
  payload += tempValid ? String(temperatureC, 2) : String("null");
  payload += "}";

  Serial.println();
  Serial.println("----- Sensor reading -----");
  Serial.printf("pH interface voltage: %.3f V | estimated pH: %.2f\n", phVoltage, phValue);
  Serial.printf("EC interface voltage: %.3f V | estimate (NOT calibrated): %.2f mS/cm\n",
                ecVoltage, ecValue);
  if (tempValid) {
    Serial.printf("Temperature: %.2f C\n", temperatureC);
  } else {
    Serial.println("Temperature: sensor not detected");
  }

  sendLoRaPacket(payload);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nSALT SPARK — Water Quality Buoy firmware starting");

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_PH_ANALOG, ADC_11db);
  analogSetPinAttenuation(PIN_EC_ANALOG, ADC_11db);

  tempSensor.begin();

  SPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_NSS);
  Serial.println("Starting SX1262 LoRa radio...");

  int state = radio.begin(LORA_FREQUENCY_MHZ);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("SX1262 ready");
    // Defaults are used for bandwidth, spreading factor and coding rate.
    // Configure these identically on the receiver.
  } else {
    Serial.print("SX1262 initialization failed, code: ");
    Serial.println(state);
    Serial.println("Check pin mapping, module type and power. Sampling will continue, but TX may fail.");
  }

  sampleAndTransmit();
  lastSampleMs = millis();
}

void loop() {
  if (millis() - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = millis();
    sampleAndTransmit();
  }
}
