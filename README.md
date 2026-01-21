# PolyConLab_FW

Repository that holds all the firmware and utilities related to the ESP32-based PLC from Arduino Shields. Part of a Final Degree Project for Facultad de Ingeniería, UDELAR.
This project is part of a Final Degree Project for the Facultad de Ingeniería, UDELAR. For more information about the project, please refer to the documentation provided in the repository or contact the authors directly.

## Table of Contents

- [1. Prerequisites and Introduction](#1-prerequisites-and-introduction)
- [2. Setting Up the Project](#2-setting-up-the-project)
- [3. Building the Firmware](#3-building-the-firmware)
- [4. Flashing the Firmware](#4-flashing-the-firmware)
- [5. Utilities](#5-utilities)
  - [5.1. Testing and Simulation](#51-testing-and-simulation)
  - [5.2. Pre-commit Hooks](#52-pre-commit-hooks)
    - [5.2.1. Installation](#521-installation)
    - [5.2.2. Usage](#522-usage)
- [6. Known Issues](#6-known-issues)
  - [6.1. Unable to find COM Port on Windows](#61-unable-to-find-com-port-on-windows)
  - [6.2. Industrial Shields Library Issues](#62-industrial-shields-library-issues)
  - [6.3. Testing and Simulation](#63-testing-and-simulation)
    - [6.3.1. Arduino Header Dependencies](#631-arduino-header-dependencies)
- [7. Contact Information](#7-contact-information)

## 1. Prerequisites and Introduction

Before you begin, ensure you have met the following requirements, regarding your development environment:

- You have installed the following extensions in Visual Studio Code:
  - PlatformIO IDE by PlatformIO. Additional information can be found [here](https://docs.platformio.org/en/latest/what-is-platformio.html).
  - External Industrial Shield Library by Industrial Shields. Additional information can be found [here](https://github.com/Industrial-Shields/platform-industrialshields-esp32/tree/main). The version used for this project is v2.5.2 of the "industrialshields-boards-esp32".
  - C/C++ Extension Pack by Microsoft.
  - Serial Monitor by Microsoft.
  - Clang-Format by Xaver Hellauer.
  - Doxygen by Baptist BENOIST.
  - Doxygen Documentation Generator by Christoph Schlosser.

Other extensions may be useful, but these are essential for development.
Refering to the Industrial Shields library, it is only available through PlatformIO or directly from Arduino IDE. For this reason, PlatformIO is the recommended IDE for this project on top of Visual Studio Code. It is also recommended to have the Utilities installed that we recommend for the project, as described in the [Utilities](#5-utilities) section.
Furthermore, since the library was initially designed for Arduino IDE, the PlatformIO project configuration file (`platformio.ini`) needs to use the Arduino framework for ESP32.

## 2. Setting Up the Project

1. Open Visual Studio Code.
2. Make sure you have Git installed on your system.
3. Install the necessary extensions in Visual Studio Code mentioned in the [Prerequisites and Introduction](#1-prerequisites-and-introduction) section.
4. Open the terminal in Visual Studio Code (Ctrl + `).
5. Navigate to the directory where you want to clone the repository.
6. Clone the repository.

## 3. Building the Firmware

Since this project uses PlatformIO as the build system, building the firmware is straightforward. Follow these steps:

1. Open Visual Studio Code.
2. Open the project folder.
3. Open the PlatformIO Home by clicking on the PlatformIO icon in the left sidebar.
4. Click on "Projects" and select your platform.
5. Click on the "Build" button (checkmark icon) in the bottom bar or use the command palette (Ctrl+Shift+P) and type "PlatformIO: Build".

**Note**: The first time you build the project, PlatformIO will download all necessary libraries and dependencies. This may take some time depending on your internet connection. For known issues during the build process, please refer to secction

## 4. Flashing the Firmware

To flash the firmware onto the ESP32-based PLC, follow these steps:

1. Connect your ESP32-based PLC to your computer via USB.
2. Open Visual Studio Code.
3. Open the PlatformIO Home by clicking on the PlatformIO icon in the left sidebar.
4. Click on "Projects" and select your project.
5. Click on the "Upload" button (right arrow icon) in the bottom bar or use the command palette (Ctrl+Shift+P) and type "PlatformIO: Upload".

## 5. Utilities

This section describes the utilities used in this project, which help maintain code quality and facilitate testing.

### 5.1. Testing and Simulation

For information regarding testing and simulation, please refer to the [Testing Directory and Simulation](test/README.md) document located in the `test` folder of this repository.

### 5.2. Pre-commit Hooks, Formatting, and Linting

This project uses pre-commit hooks to maintain code quality and consistency. The hooks automatically check and format your code before each commit. These pre-commit hooks use `clang-format` for code formatting and `cppcheck` for static code analysis, while, on the other hand, PlatformIO has a built-in `clang-tidy` that can be used for additional static analysis if desired. It can be accessed through the PlatformIO IDE extension in Visual Studio Code.

#### 5.2.1. Installation

1. Install Python (if not already installed). You can download it from [python.org](https://www.python.org/downloads/).
2. Go to [https://github.com/pocc/pre-commit-hooks](https://github.com/pocc/pre-commit-hooks) and go to the "Information about Commands -> Installation" section for more details on how to install the following dependencies:
  -llvm
  -cppcheck

3. Install pre-commit and clang-format (if not already installed):

   ```powershell
   python -m pip install pre-commit clang-format
   ```

4. Install the git hook scripts:

   ```powershell
   pre-commit install
   ```

5. Restart your computer to ensure all changes take effect.

We suggest running a virtual environment for Python to avoid conflicts with other packages.

#### 5.2.2. Usage

Pre-commit will now run automatically every time you `git commit`. It checks your staged files and blocks the commit if any issues are found.

You can also run pre-commit manually:

```powershell
# Run on all files
pre-commit run --all-files

# Run on specific files
pre-commit run --files src/main.cpp

# Run only on staged files
pre-commit run
```

Make sure to run these commands in the terminal where Python and pre-commit are installed and **before committing your changes, it is very important to keep the codebase clean and consistent**.
If these commands do not run or the package is not found even after installation, run the commands by  prefixing them with `python -m`, like so:

```powershell
python -m pre-commit run --all-files
```

## 6. Known Issues

### 6.1. Unable to find COM Port on Windows

If you are using Windows and PlatformIO is unable to find the COM port of your ESP32-based PLC, ensure that you have the necessary drivers installed. You may need to install the CP210x USB to UART Bridge VCP Drivers, which can be found on the Silicon Labs [website](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers).

### 6.2. Industrial Shields Library Issues

If you encounter issues related to the Industrial Shields library, the only known method of fixing this issue is by deleting the `.pio` folder in the project directory and also deleting the `.platformio` folder. This folder is usually located in the user's home directory.
The next time you build the project, PlatformIO will re-download all necessary libraries and dependencies, this may take some time depending on your internet connection.

### 6.3. Testing and Simulation

#### 6.3.1. Arduino Header Dependencies

If you encounter compilation errors related to missing Arduino.h or Arduino libraries during tests:

1. Check if the included header truly requires Arduino functionality
2. Consider refactoring the code to separate Arduino-dependent and independent logic
3. Use conditional compilation (`#ifdef ARDUINO`) to conditionally include Arduino-specific code
4. For pure unit tests (host simulation), use mock objects or pure C++ implementations

### 7. Contact Information

For any questions or further information, please reach out to:

- **Giuliano Turpía**: [giuliano.turpia@fing.edu.uy](mailto:giuliano.turpia@fing.edu.uy)
- **Sofía Miranda**: [sofia.miranda@fing.edu.uy](mailto:sofia.miranda@fing.edu.uy)
- **Francisco Galietta**: [francisco.galietta@fing.edu.uy](mailto:francisco.galietta@fing.edu.uy)
