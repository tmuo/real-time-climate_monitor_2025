# Arduino Climate Monitor - Hardware Documentation

## Component List

| Component | Model/Type | Quantity | Notes |
|-----------|------------|----------|-------|
| Microcontroller | Arduino Uno/Nano | 1 | 5V, 16MHz ATmega328P |
| CO2 Sensor | Sensirion SCD30 | 1 | I2C address 0x61 |
| OLED Display | SH1106 128x64 | 1 | I2C (usually 0x3C) |
| RTC Module | DS3231 | 1 | I2C address 0x68 |
| SD Card Module | Generic SPI | 1 | **Requires 3.3V or level shifter** |
| Light Sensor | Photoresistor/LDR | 1 | With 10kΩ resistor |
| LED (Yellow) | 5mm LED | 1 | SD card status indicator |
| LED (Green) | 5mm LED | 1 | Lights status indicator |
| Resistors | 220-330Ω | 2 | For LED current limiting |
| Resistor | 10kΩ | 1 | For light sensor voltage divider |
| Breadboard | Standard | 1 | Or custom PCB |
| Jumper Wires | Various | ~20 | |

## Pin Assignment Chart

### Arduino Uno Pin Mapping

```
Arduino Uno
┌─────────────────────┐
│                     │
│   DIGITAL PINS      │
│   ┌─────────────┐   │
│   │ 0  RX       │   │ (Serial - leave free)
│   │ 1  TX       │   │ (Serial - leave free)
│   │ 2  ●────────┼───┼─→ SD Status LED (Yellow) + 220Ω resistor
│   │ 3  ●────────┼───┼─→ Lights Status LED (Green) + 220Ω resistor
│   │ 4           │   │
│   │ 5           │   │
│   │ 6           │   │
│   │ 7           │   │
│   │ 8           │   │
│   │ 9           │   │
│   │ 10 ●────────┼───┼─→ SD Card CS (Chip Select)
│   │ 11 ●────────┼───┼─→ SD Card MOSI
│   │ 12 ●────────┼───┼─→ SD Card MISO
│   │ 13 ●────────┼───┼─→ SD Card SCK
│   └─────────────┘   │
│                     │
│   ANALOG PINS       │
│   ┌─────────────┐   │
│   │ A0 ●────────┼───┼─→ Light Sensor (photoresistor + 10kΩ divider)
│   │ A1          │   │ (used for random seed)
│   │ A2          │   │
│   │ A3          │   │
│   │ A4 ●────────┼───┼─→ I2C SDA (SCD30, OLED, RTC)
│   │ A5 ●────────┼───┼─→ I2C SCL (SCD30, OLED, RTC)
│   └─────────────┘   │
│                     │
│   POWER             │
│   ┌─────────────┐   │
│   │ 5V  ●───────┼───┼─→ Power to all 5V modules
│   │ 3.3V ●──────┼───┼─→ Power to SD card (if 3.3V only)
│   │ GND ●───────┼───┼─→ Common Ground
│   └─────────────┘   │
└─────────────────────┘
```

## Detailed Connection Tables

### I2C Bus (Shared)

| Arduino Pin | SCD30 | SH1106 OLED | DS3231 RTC |
|-------------|-------|-------------|------------|
| A4 (SDA) | SDA | SDA | SDA |
| A5 (SCL) | SCL | SCL | SCL |
| 5V | VCC | VCC | VCC |
| GND | GND | GND | GND |

**⚠️ CRITICAL**: All three I2C devices share the same bus. Most modules include pull-up resistors, but if you experience communication issues, check with a multimeter that pull-ups exist (~4.7kΩ to 5V).

### SPI Bus (SD Card)

| Arduino Pin | SD Card Module | Function |
|-------------|----------------|----------|
| 10 | CS | Chip Select |
| 11 | MOSI | Master Out Slave In |
| 12 | MISO | Master In Slave Out |
| 13 | SCK | Serial Clock |
| **3.3V** | VCC | Power (**NOT 5V!**) |
| GND | GND | Ground |

**⚠️ VOLTAGE WARNING**: Most SD card modules operate at 3.3V only. Connecting 5V can permanently damage the SD card. Options:
1. Use Arduino's 3.3V pin for SD module VCC
2. Use a SD module with built-in level shifter/regulator
3. Add external level shifters on MOSI, SCK, and CS lines

### Light Sensor Circuit

| Component | Connection |
|-----------|------------|
| Photoresistor (LDR) | One end to 5V |
| Photoresistor (LDR) | Other end to A0 |
| 10kΩ Resistor | From A0 to GND |
| Arduino A0 | Middle point of voltage divider |

```
Circuit Diagram:
    5V
     │
     │
    [LDR] Photoresistor
     │
     ├──────→ A0 (Arduino)
     │
    [10kΩ]
     │
    GND
```

**Theory**: Light changes LDR resistance. Voltage divider creates variable voltage at A0. Code threshold is 950 (out of 1023), meaning darker = higher value.

### LED Indicators

| Function | Pin | LED Color | Resistor | Connection |
|----------|-----|-----------|----------|------------|
| SD Status | D2 | Yellow | 220-330Ω | D2 → Resistor → LED+ → LED- → GND |
| Lights Status | D3 | Green | 220-330Ω | D3 → Resistor → LED+ → LED- → GND |

```
LED Circuit:
Pin D2/D3 ──→ [220Ω] ──→ |▷|LED ──→ GND
                        (long leg)
```

## Complete Wiring Diagram (Breadboard Layout)

```
                         ┌─────────────────┐
                         │  Arduino Uno    │
                         │                 │
                   ┌─────┤ A4 (SDA)        │
                   │  ┌──┤ A5 (SCL)        │
                   │  │  │                 │
         I2C BUS   │  │  │  D2 ────────────┼──→ [220Ω] → Yellow LED → GND
         ========  │  │  │  D3 ────────────┼──→ [220Ω] → Green LED → GND
            │      │  │  │                 │
    ┌───────┼──────┼──┼──┤  D10 ───────────┼──→ SD CS
    │       │      │  │  │  D11 ───────────┼──→ SD MOSI
    │       │      │  │  │  D12 ───────────┼──→ SD MISO
┌───┴──┐ ┌──┴───┐ │  │  │  D13 ───────────┼──→ SD SCK
│SCD30 │ │SH1106│ │  │  │                 │
│ CO2  │ │ OLED │ │  │  │  A0 ────────────┼──→ Light Sensor (see below)
│      │ │      │ │  │  │                 │
│ SDA──┼─┼──SDA─┼─┘  │  │  5V ────────────┼──→ All 5V devices
│ SCL──┼─┼──SCL─┼────┘  │  3.3V ──────────┼──→ SD Card VCC
│ VCC──┼─┼──VCC─┼───┐   │  GND ───────────┼──→ Common Ground
│ GND──┼─┼──GND─┼───┼───┤                 │
└──────┘ └──────┘   │   └─────────────────┘
                    │
              ┌─────┴─────┐
              │  DS3231   │
              │    RTC    │
              │           │
              │  SDA ─────┘
              │  SCL ─────┘
              │  VCC ─────┘
              │  GND ─────┘
              └───────────┘

Light Sensor Detail:
    5V ──→ [LDR] ──┬──→ A0
                   │
                  [10kΩ]
                   │
                  GND
```

## Critical Assembly Notes

### ⚠️ BEFORE YOU POWER ON

1. **SD Card Voltage**: Double-check SD module voltage rating. If unsure, use 3.3V pin.

2. **I2C Pull-ups**: The code sets I2C to 100kHz for reliability. Each module typically has its own pull-ups, but having multiple can actually help.

3. **Power Budget**: 
   - SCD30: ~75mA peak
   - OLED: ~20mA
   - RTC: ~1mA
   - SD card: ~100mA peak during writes
   - **Total: ~200mA peak**
   - Arduino USB can provide 500mA, but use external power if experiencing resets

4. **Ground Loops**: Ensure all GND connections are solid. Poor grounds cause I2C errors.

### LED Resistor Calculation

The code doesn't specify, but here's why 220-330Ω is recommended:

```
For typical 5mm LED (20mA max, ~2V forward voltage):
R = (Vsupply - Vled) / Iled
R = (5V - 2V) / 0.020A = 150Ω minimum

Safe range: 220-330Ω gives 10-14mA (bright enough, won't burn out)
```

**Without resistors, LEDs will fail quickly!**

## Testing Procedure

### Phase 1: Power Test (No SD Card)
1. Connect only Arduino, I2C devices, and LEDs
2. Upload code
3. Check Serial Monitor for:
   - SCD30 firmware version
   - RTC initialization
   - SD error (expected - no card yet)

### Phase 2: I2C Communication
```
Expected Serial Output:
--- RTC Initialization ---
RTC found!
RTC Time: 2025-XX-XX XX:XX:XX
SCD30 Firmware - major: 3  minor: 66
```

If I2C fails:
- Check wiring
- Verify pull-up resistors (measure ~4.7kΩ from SDA/SCL to 5V)
- Try slower I2C: change `Wire.setClock(50000);`

### Phase 3: SD Card
1. Insert formatted FAT32 SD card (≤32GB)
2. Connect SD module (3.3V!)
3. Reboot Arduino
4. Should see: `SD card initialized successfully`

### Phase 4: Light Sensor Calibration
1. Check Serial Monitor for light value
2. Adjust `lightThreshold` in code if needed:
   - Current: 950 (lights ON when value ≤ 950)
   - Darker room needs higher threshold
   - Brighter room needs lower threshold

## Common Issues & Solutions

| Problem | Likely Cause | Solution |
|---------|--------------|----------|
| "RTC not found" | Wiring/address conflict | Check SDA/SCL, try I2C scanner sketch |
| SD writes fail | Voltage/wiring | Verify 3.3V power, check CS pin |
| Display blank | Contrast/address | Display auto-adjusts brightness; check I2C address |
| I2C hangs | Bus conflict | Add `recoverI2C()` calls (already in code) |
| Sensor reads 0 | Initialization fail | Power cycle, check 5V supply |
| LEDs don't light | Missing resistors/backwards | Check polarity (long leg = +), add resistors |

## Optional Improvements

1. **Add reset button** between Arduino RESET and GND
2. **Enclosure ventilation** required for accurate CO2/temp readings
3. **External 5V/2A power supply** for stability
4. **I2C bus extender** if wires >30cm
5. **Real-time calibration button** on D4 instead of serial commands

## Firmware Configuration

After assembly, configure via Serial Monitor (115200 baud):

```
Commands:
AUTOSET              - Set RTC to compile time
SETTIME 2025 12 21 15 30 00  - Set RTC manually
CAL                  - Calibrate CO2 to 400ppm
CAL 450             - Calibrate to custom ppm
SDCHECK             - Reinitialize SD card
GETTIME             - Display current time
```

## Pin Summary Quick Reference

```
DIGITAL PINS:
D2  → Yellow LED (SD status) + 220Ω resistor to GND
D3  → Green LED (Lights status) + 220Ω resistor to GND
D10 → SD Card CS
D11 → SD Card MOSI
D12 → SD Card MISO
D13 → SD Card SCK

ANALOG PINS:
A0  → Light sensor (LDR voltage divider)
A4  → I2C SDA (SCD30, OLED, RTC)
A5  → I2C SCL (SCD30, OLED, RTC)

POWER:
5V   → SCD30, OLED, RTC, LDR top, LED resistors
3.3V → SD Card Module VCC
GND  → All modules, LDR bottom, LED cathodes
```

---

**Important**: This project involves careful voltage management and multiple communication buses. The SD card voltage issue is the most common point of failure—verify your module's requirements before connecting power.
