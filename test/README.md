# Testing Directory and Simulation

This directory hosts both hardware and host-side tests, organized in subfolders. We use the following structure for tests:

```bash
test/
├── hardware/
│   ├── test_name_of_module/
└── virtual/
    ├── test_name_of_module/
```

Each test folder contains a C++ test file that can be compiled and run on the platform of preference, that being a PC or the target hardware.
Tests now use the doctest framework (header-only, C++11) integrated with PlatformIO. doctest is lightweight and works for both native (simulation) and Arduino targets when built through PlatformIO.
The tests executions depend on the target platform, as described below. In all cases, ensure you are in a PlatformIO CLI terminal environment.

## Table of Contents

- [1. Prerequisites](#1-prerequisites)
  - [1.1. Setting Up GCC](#11-setting-up-gcc)
    - [1.1.1. Windows](#111-windows)
    - [1.1.2. macOS](#112-macos)
    - [1.1.3. Linux](#113-linux)
- [2. Testing](#2-testing)
  - [2.1. Virtual Tests](#21-virtual-tests)
  - [2.2. Target Hardware Tests](#22-target-hardware-tests)

## 1. Prerequisites

### 1.1. Setting Up GCC

If you do not have GCC installed on your system, follow these steps:

#### 1.1.1. Windows

1. Download MinGW-w64 from [https://www.mingw-w64.org/](https://www.mingw-w64.org/)
2. Run the installer and follow the installation wizard, installing it in `C:\msys64` or another preferred location
3. Add the MinGW bin directory to your system PATH (typically `C:\msys64\mingw64\bin`)
4. Restart your system.
5. Verify installation by opening a terminal and running:

   ```bash
   gcc --version
   ```

#### 1.1.2. macOS

1. Install Xcode Command Line Tools:

   ```bash
   xcode-select --install
   ```

2. Follow the prompts to complete the installation
3. Verify installation:

   ```bash
   gcc --version
   ```

#### 1.1.3. Linux

Most Linux distributions come with GCC pre-installed. If not, use your package manager to install it.

## 2. Testing

The tests are divided into two categories: simulation tests that run on the host machine, and tests that run on the actual target hardware.
The purpose of this division is to allow for quick verification of logic and functionality without the need for flashing the target device, as well as to perform comprehensive tests on the actual hardware when necessary. In particular, virtual tests can be executed in both PLC and PC environments, while hardware tests can only be run on the target PLC. The "virtual" tests as mentioned can run on both environments, but are primarily intended for testing in a PC environment, since the abstractions that take place in the HAL layer allow for this flexibility.

### Important Testing Note

**Virtual (native) tests run modules in isolation without FreeRTOS tasks.** This means:

- Tests validate individual module behavior (e.g., StateMachine transitions, HAL operations)
- Tests do NOT run the full application with concurrent tasks
- Tests do NOT include the event queue or task scheduling from `main.cpp`
- For integration testing with FreeRTOS, you must test on hardware targets.

## 2.1. Virtual Tests

These tests run on the host machine using the native platform environment

```bash
pio test -e native --without-uploading
```

This will run all tests in the `test/virtual/` directory. The `--without-uploading` flag is used to skip any upload steps, as these tests do not require flashing to hardware.

To run a specific test file in simulation mode, use:

```bash
pio test -e native --without-uploading -f "test_name_of_module"
```

Where `test_name_of_module` is the name of the test folder containing the desired test file.

### 2.2. Target Hardware Tests

The tests in this category are executed on the actual target hardware (ESP32-based PLC), or any other supported target defined in the `platformio.ini` file. Apart from the PLC, other targets can be defined testing IoT nodes purposes, for instance, an ESP32 DevKitC board to test Wi-Fi connectivity features, etc.

To run all tests on the actual target hardware, use the following command:

```bash
pio test -e <env_name> --upload-port <port_name>
```

This will run all tests in the `test/hardware/` directory on the specified target hardware.

Where `<env_name>` corresponds to the environment defined in `platformio.ini` for the target hardware, and `<port_name>` is the serial port connected to the target device.

To run a specific test file on the target hardware, use:

```bash
pio test -e <env_name> --upload-port <port_name> -f "test_name_of_module"
```

Where `test_name_of_module` is the name of the test folder containing the desired test file.
