# Wiring & Pinout

## Power Distribution
| From | To |
|---|---|
| 12V Battery (+) | L298N +12V input |
| 12V Battery (+) | LM2596 Buck Converter IN+ |
| 12V Battery (–) | L298N GND |
| 12V Battery (–) | LM2596 Buck Converter IN– |
| LM2596 OUT+ (5V) | ESP32 VIN, all sensor VCC pins |
| LM2596 OUT– | Common Ground Rail |

**All grounds (battery, buck converter, ESP32, L298N, sensors) must be tied to one common rail.**

## L298N (Propulsion) → ESP32
| L298N Pin | ESP32 GPIO |
|---|---|
| ENA | 14 |
| IN1 | 27 |
| IN2 | 26 |
| ENB | 25 |
| IN3 | 33 |
| IN4 | 13 |

## Conveyor Belt Motor Driver → ESP32
| Pin | ESP32 GPIO |
|---|---|
| PWM | 12 |
| IN1 | 15 |
| IN2 | 2 |

## HC-SR04 Ultrasonic Sensor
| Pin | Connects to |
|---|---|
| VCC | 5V rail |
| TRIG | GPIO 5 |
| ECHO | GPIO 18 (via 1kΩ + 2kΩ voltage divider — 5V→3.3V protection) |
| GND | Common Ground |

## IR Reflectance Sensor
| Pin | Connects to |
|---|---|
| VCC | 5V rail |
| OUT | GPIO 4 |
| GND | Common Ground |

## Water Quality Sensors
| Sensor | Pin | ESP32 GPIO |
|---|---|---|
| Turbidity (pre-filter) | AOUT | 34 |
| Turbidity (post-filter) | AOUT | 35 |
| pH Sensor | AOUT | 39 |

**Note:** GPIO 34, 35, 39 are ADC1 input-only pins — required since ADC2 conflicts with WiFi.

## Known Fix
GPIO 32 was originally assigned to both L298N IN4 and the pH sensor — reassigned IN4 to GPIO 13 to resolve the conflict.
