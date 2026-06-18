# Testing Directory

This directory hosts hardware tests, organized in subfolders with the following structure:

```bash
test/
└── hardware/
    └── test_name_of_module/
```

Each test folder contains a C++ test file that is compiled and run on the target hardware.
Tests use the doctest framework (header-only, C++11) integrated with PlatformIO.

## Table of Contents

- [1. Testing on Target Hardware](#1-testing-on-target-hardware)

## 1. Testing on Target Hardware

The tests in this category are executed on the actual target hardware (ESP32-based PLC),
or any other supported target defined in the `platformio.ini` file.

To run all tests on the target hardware:

```bash
pio test -e <env_name> --upload-port <port_name>
```

Where `<env_name>` corresponds to the environment defined in `platformio.ini` for the target
hardware, and `<port_name>` is the serial port connected to the target device.

To run a specific test file on the target hardware:

```bash
pio test -e <env_name> --upload-port <port_name> -f "test_name_of_module"
```

Where `test_name_of_module` is the name of the test folder containing the desired test file.
