# DC Motor Control App

A comprehensive application for controlling DC motors with microcontroller support, built with C++ and Arduino.

## 📋 Overview

This project provides a complete solution for DC motor control, combining a C++ desktop application with Arduino firmware for hardware interaction. The application allows for precise motor speed and direction control through an intuitive interface.

## 🏗️ Project Structure

```
DCMotorControlApp/
├── DCMotorControl.slnx          # Solution file for Visual Studio
├── DCMotorControlApp/           # Main application folder
├── dc-motor-control.ino         # Arduino firmware for motor control
├── .gitattributes              # Git attributes configuration
├── .gitignore                  # Git ignore rules
└── README.md                   # This file
```

## 🛠️ Technologies Used

- **C++** - Main application language
- **Arduino** - Microcontroller firmware
- **Visual Studio** - Development environment

## 🚀 Getting Started

### Prerequisites

- Visual Studio 2019 or later (for C++ application)
- Arduino IDE (for firmware upload)
- Compatible microcontroller (Arduino Uno, Mega, or similar)
- DC motor with motor driver module (L298N or equivalent)

### Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/MohammadBaroutjy/DCMotorControlApp.git
   cd DCMotorControlApp
   ```

2. **For the C++ Application:**
   - Open `DCMotorControl.slnx` in Visual Studio
   - Build the solution

3. **For Arduino Firmware:**
   - Open Arduino IDE
   - Load `dc-motor-control.ino`
   - Select your board and COM port
   - Upload the sketch to your microcontroller

## 🎮 Features

- DC motor speed control
- Direction control (forward/reverse)
- Real-time feedback and monitoring
- User-friendly interface
- Serial communication with microcontroller

## 💻 Hardware Connections

The Arduino firmware expects the following connections:

- Motor control pins via PWM output
- Direction control pins for motor direction
- Optional feedback pins for speed monitoring

*Refer to the Arduino sketch comments for detailed pin configuration.*

## 📝 Usage

1. **Connect Hardware:**
   - Wire your DC motor and driver to the Arduino according to the firmware pin configuration
   - Connect Arduino to your computer via USB

2. **Run Application:**
   - Start the C++ desktop application
   - Select the COM port of your connected Arduino
   - Use controls to adjust motor speed and direction

3. **Monitor:**
   - View real-time motor status
   - Adjust parameters as needed

## 🐛 Troubleshooting

- **Motor not responding:** Check serial connection and verify Arduino firmware is uploaded
- **Connection issues:** Ensure correct COM port is selected
- **Inconsistent speed:** Verify motor driver power supply

## 📄 License

This project is provided as-is. No specific license is specified at this time.

## 👨‍💻 Author

[MohammadBaroutjy](https://github.com/MohammadBaroutjy)

## 🤝 Contributing

Contributions are welcome! Feel free to:
- Report bugs and issues
- Suggest improvements
- Submit pull requests

## 📞 Support

For questions or issues, please open an issue on the [GitHub repository](https://github.com/MohammadBaroutjy/DCMotorControlApp/issues).

---

**Last Updated:** June 2026
