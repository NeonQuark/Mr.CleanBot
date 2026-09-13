# Autonomous Adaptive Water Surface Cleaning Robot

An ESP32-based autonomous catamaran robot that navigates a pond using ultrasonic obstacle avoidance, collects floating debris via a conveyor-belt mechanism, passively filters collected water, and monitors water quality in real time — with adaptive collection behavior and IoT data upload.

## Status: In Development (Build in progress)

## Features
- **Autonomous navigation** — ultrasonic obstacle avoidance, tuned to distinguish tall obstacles from low floating debris
- **Debris collection** — conveyor-belt mechanism with debris-density-adaptive speed control (IR sensor fusion)
- **Water filtration** — passive two-stage mesh filter, turbidity measured before/after
- **Water Quality Index (WQI)** — weighted classification (Safe / Moderate / Polluted) from pH + turbidity readings
- **IoT monitoring** — live sensor data + WQI status uploaded via WiFi to a dashboard
- **Non-blocking control loop** — safety-critical navigation never interrupted by data upload

## Hardware
- ESP32 Dev Board
- L298N Motor Driver (x2, propulsion + conveyor)
- HC-SR04 Ultrasonic Sensor
- IR Reflectance Sensor
- Turbidity Sensor (x2 — pre/post filter)
- pH Sensor
- 12V Li-ion Battery + LM2596 Buck Converter

## Wiring
See `/docs/wiring.md` for full pinout and circuit diagram.

## Repository Structure
```
skimmer_boat/
├── skimmer_boat.ino    # Main firmware
├── docs/
│   └── wiring.md        # Full pinout + circuit notes
└── README.md
```

## Build Timeline
- [x] System architecture & wiring design
- [x] Firmware — motor control, obstacle avoidance
- [x] Firmware — debris-density-adaptive collection logic
- [x] Firmware — WQI classification + IoT upload
- [ ] Hull assembly & waterproofing
- [ ] Sensor calibration (real-water testing)
- [ ] Full system integration test
- [ ] Demo & documentation

## Author
[Your Name] — B.Tech ECE, [College Name]

## License
MIT
