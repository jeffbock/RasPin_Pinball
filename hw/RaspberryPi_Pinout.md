# Raspberry Pi 5 GPIO Pinout for Pinball System

This document details all the GPIO pin connections required to hook up the pinball hardware to the Raspberry Pi 5 header. The pin assignments are defined in `Pinball_IO.cpp`.

## Active Low Signal Convention

**All pins are electrically designed to be active (PB_ON) low.** This means:
- Pressing a button connects the signal to ground
- Output LEDs turn on when set to low
- All other various circuitry considers a low pin state as being active from the software perspective

## Quick Reference Table

| Function | BCM GPIO | Physical Pin | Direction | Pull |
|----------|----------|--------------|-----------|------|
| **Player Controls** |
| Left Flipper | GPIO 27 | 13 | Input | Pull-up |
| Right Flipper | GPIO 17 | 11 | Input | Pull-up |
| Left Activate | GPIO 5 | 29 | Input | Pull-up |
| Right Activate | GPIO 22 | 15 | Input | Pull-up |
| Start Button | GPIO 6 | 31 | Input | Pull-up |
| Reset Button | GPIO 24 | 18 | Input | Pull-up |
| **Status LED** |
| Start LED | GPIO 23 | 16 | Output | N/A |
| **Power** |
| +3.3V | N/A | 1, 17 | Power | N/A |
| +5V | N/A | 2, 4 | Power | N/A |
| Ground | N/A | 6, 9, 14, 20, 25, 30, 34, 39 | Ground | N/A |

## Detailed Pin Diagram

```
 +-----+-----+---------+------+---+---Pi 5--+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 |     |     |    3.3v | PWR  |   |  1 || 2  |   | PWR  | 5v      |     |     |
 |   2 |   8 |   SDA.1 |  I2C | 1 |  3 || 4  |   | PWR  | 5v      |     |     |
 |   3 |   9 |   SCL.1 |  I2C | 1 |  5 || 6  |   | GND  | 0v      |     |     |
 |   4 |   7 | GPIO. 7 |   IN | 0 |  7 || 8  | 0 | IN   | TxD     | 15  | 14  |
 |     |     |      0v |      |   |  9 || 10 | 1 | IN   | RxD     | 16  | 15  |
 |  17 |   0 |RFLIP IN | IN   | 1 | 11 || 12 | 1 | IN   | GPIO. 1 | 1   | 18  |
 |  27 |   2 |LFLIP IN | IN   | 1 | 13 || 14 |   | GND  | 0v      |     |     |
 |  22 |   3 |RACT  IN | IN   | 1 | 15 || 16 | 0 | OUT  | START L | 4   | 23  |
 |     |     |    3.3v | PWR  |   | 17 || 18 | 1 | IN   | RESET IN| 5   | 24  |
 |  10 |  12 |    MOSI |   IN | 0 | 19 || 20 |   | GND  | 0v      |     |     |
 |   9 |  13 |    MISO |   IN | 0 | 21 || 22 | 1 | IN   | GPIO. 6 | 6   | 25  |
 |  11 |  14 |    SCLK |   IN | 0 | 23 || 24 | 1 | IN   | CE0     | 10  | 8   |
 |     |     |      0v |      |   | 25 || 26 | 0 | IN   | CE1     | 11  | 7   |
 |   0 |  30 |   SDA.0 |   IN | 1 | 27 || 28 | 1 | IN   | SCL.0   | 31  | 1   |
 |   5 |  21 |LACT  IN | IN   | 0 | 29 || 30 |   | GND  | 0v      |     |     |
 |   6 |  22 |START IN | IN   | 0 | 31 || 32 | 1 | IN   | GPIO.26 | 26  | 12  |
 |  13 |  23 | GPIO.23 |   IN | 1 | 33 || 34 |   | GND  | 0v      |     |     |
 |  19 |  24 | GPIO.24 |   IN | 1 | 35 || 36 | 1 | IN   | GPIO.27 | 27  | 16  |
 |  26 |  25 | GPIO.25 |   IN | 1 | 37 || 38 | 1 | IN   | GPIO.28 | 28  | 20  |
 |     |     |      0v |      |   | 39 || 40 | 1 | IN   | GPIO.29 | 29  | 21  |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+---Pi 5--+---+------+---------+-----+-----+

Legend:
  LFLIP IN  = Left Flipper Button Input (GPIO 27, Pin 13)
  RFLIP IN  = Right Flipper Button Input (GPIO 17, Pin 11)
  LACT  IN  = Left Activate Button Input (GPIO 5, Pin 29)
  RACT  IN  = Right Activate Button Input (GPIO 22, Pin 15)
  START IN  = Start Button Input (GPIO 6, Pin 31)
  RESET IN  = Reset Button Input (GPIO 24, Pin 18)
  START L   = Start LED Output (GPIO 23, Pin 16)
```

## Pin Details

### Input Pins (Buttons/Switches)

All button inputs are configured with internal pull-up resistors. Connect buttons between the GPIO pin and ground. When the button is pressed, it pulls the pin to ground (0V), which is detected as the active state.

#### Left Flipper Button
- **BCM GPIO**: 27
- **Physical Pin**: 13
- **WiringPi**: 2
- **Function**: Primary left flipper control
- **Wiring**: Connect button between GPIO 27 (Pin 13) and Ground (Pin 14)
- **Auto Output**: Triggers LED2 (IDO_LED2) when pressed

#### Right Flipper Button
- **BCM GPIO**: 17
- **Physical Pin**: 11
- **WiringPi**: 0
- **Function**: Primary right flipper control
- **Wiring**: Connect button between GPIO 17 (Pin 11) and Ground (Pin 9 or 14)
- **Auto Output**: Triggers LED3 (IDO_LED3) when pressed

#### Left Activate Button
- **BCM GPIO**: 5
- **Physical Pin**: 29
- **WiringPi**: 21
- **Function**: Secondary left control (game-specific function)
- **Wiring**: Connect button between GPIO 5 (Pin 29) and Ground (Pin 30)

#### Right Activate Button
- **BCM GPIO**: 22
- **Physical Pin**: 15
- **WiringPi**: 3
- **Function**: Secondary right control (game-specific function)
- **Wiring**: Connect button between GPIO 22 (Pin 15) and Ground (Pin 14 or 20)

#### Start Button
- **BCM GPIO**: 6
- **Physical Pin**: 31
- **WiringPi**: 22
- **Function**: Game start/select
- **Wiring**: Connect button between GPIO 6 (Pin 31) and Ground (Pin 30 or 34)

#### Reset Button
- **BCM GPIO**: 24
- **Physical Pin**: 18
- **WiringPi**: 5
- **Function**: System reset
- **Wiring**: Connect button between GPIO 24 (Pin 18) and Ground (Pin 20)

### Output Pins

#### Start LED
- **BCM GPIO**: 23
- **Physical Pin**: 16
- **WiringPi**: 4
- **Function**: Visual indicator (typically start button illumination)
- **Wiring**: Connect LED anode (long leg) through a current-limiting resistor (typically 220Ω-330Ω) to 3.3V, and cathode (short leg) to GPIO 23 (Pin 16)
- **Logic**: When GPIO 23 is set LOW (0V), the LED turns on
- **Default State**: ON

## I2C Bus Connections

The system uses I2C bus 1 for communication with expansion chips:

- **SDA (Data)**: GPIO 2, Physical Pin 3
- **SCL (Clock)**: GPIO 3, Physical Pin 5
- **Power**: 3.3V (Pin 1 or 17) or 5V (Pin 2 or 4) depending on device requirements
- **Ground**: Any ground pin (6, 9, 14, 20, 25, 30, 34, 39)

### I2C Devices Supported

The system architecture supports the following I2C expansion devices (configured in Pinball_IO.cpp):

1. **TLC59116 LED Driver** (16-channel PWM LED controller)
   - Default addresses: 0x60, 0x61, 0x62 (configurable)
   - Controls up to 48 LEDs with individual brightness
   - Group blinking/dimming support

2. **TCA9555 I/O Expander** (16-bit GPIO expander)
   - Default addresses: 0x20, 0x21, 0x22 (configurable)
   - Controls solenoids, coils, and reads sensors
   - Input mask configured per device

3. **MAX9744 Audio Amplifier**
   - Default address: 0x4B (configurable)
   - Volume control via I2C
   - 20W stereo Class D amplifier

4. **WS2812B NeoPixel LED Strips**
   - Direct GPIO control (not I2C)
   - Configurable output pin
   - RGB addressable LEDs

## Ground Connections

Multiple ground pins are available on the header. It's good practice to use the ground pin closest to your signal pins to minimize noise:

- **Pin 6**: Near I2C and top GPIO pins
- **Pin 9**: Between GPIO 4 and GPIO 17
- **Pin 14**: Near GPIO 27 (Left Flipper)
- **Pin 20**: Near GPIO 22 (Right Activate)
- **Pin 25**: Center of header
- **Pin 30**: Near GPIO 5 (Left Activate)
- **Pin 34**: Near GPIO 13
- **Pin 39**: Bottom of header

## Power Supply Notes

### 3.3V Power (Pins 1, 17)
- **Use for**: I2C pull-up resistors, 3.3V logic devices
- **Max current**: 50mA total from all 3.3V pins
- **Do NOT** connect directly to 5V devices or LEDs without proper level shifting/current limiting

### 5V Power (Pins 2, 4)
- **Use for**: I2C expansion boards with onboard regulators
- **Source**: Direct connection to Raspberry Pi 5V input
- **Warning**: No overcurrent protection - use external fuse if powering high-current devices

## Wiring Example: Basic Button Connection

For a typical button connection (e.g., Left Flipper):

```
    Raspberry Pi              Button           Ground
    Pin 13 (GPIO 27) -------- [Switch] -------- Pin 14 (GND)
```

The internal pull-up resistor in the Raspberry Pi keeps GPIO 27 at 3.3V when the button is not pressed. When pressed, the button connects GPIO 27 directly to ground, pulling it to 0V.

## Wiring Example: Start LED

For the Start LED output (active low):

```
    3.3V Power     Resistor        LED          GPIO
    Pin 1 (3.3V) --- [330Ω] --- >|+ -|> --- Pin 16 (GPIO 23)
                                (Anode) (Cathode)
```

When GPIO 23 is set to LOW (0V), current flows from 3.3V through the resistor and LED to ground, turning the LED on.

Calculate resistor value: R = (Vcc - Vf) / If
- Vcc = 3.3V (Power supply voltage)
- Vf = ~2.0V (LED forward voltage, varies by color)
- If = ~10mA (desired LED current)
- R = (3.3 - 2.0) / 0.01 = 130Ω minimum
- Use 220Ω-330Ω for safety margin

## I2C Device Wiring

For I2C expansion boards (TLC59116, TCA9555, MAX9744):

```
    Raspberry Pi                I2C Device
    Pin 3 (SDA)  ----------------  SDA
    Pin 5 (SCL)  ----------------  SCL
    Pin 1 (3.3V) ----------------  VCC (if 3.3V device)
    Pin 2 (5V)   ----------------  VCC (if 5V device with regulator)
    Pin 6 (GND)  ----------------  GND
```

Most I2C breakout boards include built-in pull-up resistors (typically 4.7kΩ) on SDA and SCL lines.

## Important Notes

1. **GPIO Voltage**: Raspberry Pi GPIO pins are **3.3V logic**. Do not connect 5V signals directly to GPIO inputs without level shifting.

2. **Current Limits**: 
   - Each GPIO pin: 16mA maximum
   - Total from all GPIO pins: 50mA maximum
   - Use external drivers (transistors/MOSFETs) for high-current devices

3. **ESD Protection**: Handle the Raspberry Pi with care. Use ESD-safe practices when making connections.

4. **Pull-up/Pull-down**: The code configures internal pull-up resistors for all button inputs. External pull-ups are not needed.

5. **Debouncing**: Input debouncing is handled in software (5ms debounce time configured in `Pinball_IO.cpp`).

6. **I2C Speed**: Default I2C bus speed is 100kHz (standard mode). Can be increased to 400kHz (fast mode) if all devices support it.

## Troubleshooting

### Buttons not responding
- Check ground connection
- Verify GPIO pin number matches physical pin
- Test continuity of button when pressed
- Check that internal pull-up is enabled in code

### LED not lighting
- Verify LED polarity (anode to 3.3V, cathode to GPIO)
- Check resistor value (should be 220Ω-330Ω)
- Test LED with multimeter in diode mode
- Verify GPIO is configured as output and set to LOW to turn LED on

### I2C devices not detected
- Run `i2cdetect -y 1` to scan I2C bus
- Check SDA/SCL connections
- Verify power supply to I2C device
- Check I2C address matches configuration in code
- Ensure I2C is enabled: `sudo raspi-config` → Interface Options → I2C

## Testing GPIO Connections

You can test GPIO pins using the command line:

```bash
# Read GPIO 27 (Left Flipper) state
gpio -g read 27

# Set GPIO 23 (Start LED) to LOW (turns LED on)
gpio -g mode 23 out
gpio -g write 23 0

# Set GPIO 23 to HIGH (turns LED off)
gpio -g write 23 1

# Show all GPIO states
gpio readall
```

## Reference Documents

- Full WiringPi pinout: `hw/wiringPiPinout.txt`
- I/O configuration: `src/Pinball_IO.cpp`
- Hardware schematics: `hw/SimpleBreakoutSchematic.png`
- WiringPi documentation: http://wiringpi.com/

## Version History

- v1.0 - Initial pinout documentation for Raspberry Pi 5
- Based on code version defined in `build_counter.txt`
