# PolyConLab_FW

Repository that holds all the firmware and utilities related to the ESP32-based PLC from Arduino Shields. Part of a Final Degree Project for Facultad de Ingeniería, UDELAR.

## Table of Contents

## 1. Prerequisites and Introduction

Before you begin, ensure you have met the following requirements, regarding your development environment:

* You have installed the following extensions in Visual Studio Code:
  * "Espressif IDF" version "5.5.1 (release version)" by Espressif Systems. Additional information can be found [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html).
  * PlatformIO IDE by PlatformIO. Additional information can be found [here](https://docs.platformio.org/en/latest/what-is-platformio.html).
  * Serial Monitor by Microsoft.
  * External Industrial Shield Library by Industrial Shields. Additional information can be found [here](https://github.com/Industrial-Shields/platform-industrialshields-esp32/tree/main).
    * The version used for this project is v2.5.2 of the "industrialshields-boards-esp32".


Other extensions may be useful, but these are essential for development. Note that PlatformIO has a built-in Smart Code Linter.
Refering to the Industrial Shields library, it is only available through PlatformIO or directly from Arduino IDE. For this reason, PlatformIO is the recommended IDE for this project on top of Visual Studio Code.
Furthermore, since the library was initially designed for Arduino IDE, the PlatformIO project configuration file (platformio.ini) needs to use the Arduino framework for ESP32.

## 2. Setting Up the Project

### 2.1. Cloning the Repository

## 3. Building the Firmware

Since this project uses PlatformIO as the build system, building the firmware is straightforward. Follow these steps:

1. Open Visual Studio Code.
2. Open the project folder.
3. Open the PlatformIO Home by clicking on the PlatformIO icon in the left sidebar.
4. Click on "Projects" and select your project.
5. Click on the "Build" button (checkmark icon) in the bottom bar or use the command palette (Ctrl+Shift+P) and type "PlatformIO: Build".

## 4. Flashing the Firmware

To flash the firmware onto the ESP32-based PLC, follow these steps:

1. Connect your ESP32-based PLC to your computer via USB.
2. Open Visual Studio Code.
3. Open the PlatformIO Home by clicking on the PlatformIO icon in the left sidebar.
4. Click on "Projects" and select your project.
5. Click on the "Upload" button (right arrow icon) in the bottom bar or use the command palette (Ctrl+Shift+P) and type "PlatformIO: Upload".

## 5. Additional Utilities

### 5.1. Pre-commit Hooks

This project uses pre-commit hooks to maintain code quality and consistency. The hooks automatically check and format your code before each commit.

#### Installation

1. Install Python (if not already installed). You can download it from [python.org](https://www.python.org/downloads/).

2. Install pre-commit (if not already installed):

   ```powershell
   python -m pip install pre-commit
   ```

3. Install the git hook scripts:

   ```powershell
   pre-commit install
   ```

#### Usage

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

If these commands do not run or the package is not found even after installation, run the commands by  prefixing them with `python -m`, like so:

```powershell
python -m pre-commit run --all-files
```

#### Configuration

The project includes a `.clang-format` file to customize code formatting style. You can modify settings such as:

- `IndentWidth`: spaces per indent level
- `ColumnLimit`: maximum line length
- `BreakBeforeBraces`: brace style (Attach, Allman, Linux, etc.)

See all available options at: https://clang.llvm.org/docs/ClangFormatStyleOptions.html

## 6. Notes

This project is part of a Final Degree Project for the Facultad de Ingeniería, UDELAR. For more information about the project, please refer to the documentation provided in the repository or contact the authors directly.

### 6.1. Contact Information

For any questions or further information, please reach out to:

- **Giuliano Turpía**: [giuliano.turpia@fing.edu.uy](mailto:giuliano.turpia@fing.edu.uy)
- **Sofía Miranda**: [sample](mailto:sample)
- **Francisco Galietta**: [sample](mailto:sample)
