# SALT SPARK — GitHub Starter Firmware

Autonomous seawater-powered water-quality monitoring buoy using an ESP32-S3, analog pH and EC interface boards, a DS18B20 temperature sensor, and an SX1262 LoRa radio.

## What this starter does
- Reads analog voltage from a pH interface board.
- Reads analog voltage from an EC/salinity interface board.
- Reads water temperature from a DS18B20.
- Sends a JSON-like data packet over SX1262 LoRa every 30 seconds.
- Prints readings and radio status to the Serial Monitor.

## Hardware assumed
- ESP32-S3 development board
- pH probe + compatible signal-conditioning/interface board with analog output
- EC/salinity probe + compatible analog interface board
- DS18B20 temperature sensor (with 4.7 kΩ pull-up from DATA to 3.3 V)
- SX1262 LoRa module with antenna
- Stable regulated supply for testing

**Do not connect raw pH probes directly to an ESP32 ADC.** A proper high-impedance pH interface board is required. Check the analog output voltage of every sensor board before connecting it to the ESP32. ESP32-S3 GPIOs are not 5 V tolerant.

## Pin mapping
The pin numbers in `src/main.cpp` are examples only. Check the pinout for your exact ESP32-S3 board and change the constants before wiring.

| Signal | Default GPIO in this starter |
|---|---:|
| pH analog output | 1 |
| EC analog output | 2 |
| DS18B20 data | 4 |
| SX1262 SCK | 12 |
| SX1262 MISO | 13 |
| SX1262 MOSI | 11 |
| SX1262 NSS/CS | 10 |
| SX1262 DIO1 | 9 |
| SX1262 RESET | 8 |
| SX1262 BUSY | 7 |

Use the correct voltage levels and common ground. Connect the LoRa antenna before transmitting.

## Build using PlatformIO
1. Install Visual Studio Code and the PlatformIO IDE extension, or use an existing PlatformIO setup.
2. Open this folder as a PlatformIO project.
3. Check/change the board and pin constants.
4. Build and upload to the ESP32-S3.
5. Open Serial Monitor at **115200 baud**.

Dependencies are declared in `platformio.ini`:
- RadioLib
- OneWire
- DallasTemperature

## Calibration — required before trusting measurements
### pH
The pH slope and offset in the code are placeholders. Calibrate with suitable pH buffer solutions (commonly pH 7 and pH 4 or pH 10), following the exact interface-board instructions. Replace `PH_SLOPE` and `PH_OFFSET` with values derived from your measured voltages. Verify against a reference meter.

### EC / salinity
The EC conversion is intentionally marked **uncalibrated**. EC probe modules use different circuits and conversion curves, so a universal voltage-to-salinity equation is not safe to assume. Calibrate using a known conductivity standard and the sensor manufacturer's method. EC is not identical to salinity; convert to salinity only using a method appropriate for the probe and water conditions.

### Temperature
The code reads DS18B20 temperature in °C. Waterproof probe packaging and response time vary.

## LoRa notes
- This is a transmitter-only starter. A second LoRa node or gateway needs matching radio settings and receiver firmware.
- Frequency `866.5 MHz` is an example within the commonly used Indian 865–867 MHz range; verify current local rules, duty-cycle/power limits, and your module's frequency support before deployment.
- This is not LoRaWAN. It sends a simple point-to-point packet.
- Use a suitable antenna and never transmit with an antenna-less RF module.

## Power-system integration
For initial software testing, power the ESP32-S3 and sensors from a stable regulated USB supply. Test the sensors and radio first. Only connect the magnesium-air cell, energy harvester/boost stage, supercapacitor and regulator after measuring the cell's loaded voltage/current and verifying the converter's startup and output requirements. Do not connect a raw experimental cell directly to the ESP32.

## Prototype limitations
This is starter firmware, not a validated marine monitoring instrument. Calibrate and compare sensor readings with reference instruments before drawing environmental conclusions. Waterproofing, corrosion protection, power budgeting, logging, packet-loss handling, and receiver/dashboard software are future tasks.
