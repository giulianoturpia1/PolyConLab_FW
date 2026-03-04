# Libraries Directory

This directory contains various libraries used by the project. Each library is organized in its own subdirectory, which includes source code, documentation, and any necessary configuration files.
For more detailed information about specific libraries, please refer to their respective README files located within each subdirectory. This is true for most of the libraries, except for the Hardware Abstraction Layer (HAL) library, which is documented below, since they span across multiple subdirectories that can not be separated in nested folders due to PlatformIO limitations.

As a general rule, each library follows the structure below, which PlatformIO recognizes automatically:

```text
lib/
├── Module1/
│   ├── Module1.cpp
│   └── Module1.hpp
├── Module2/
│   ├── Module2.cpp
│   └── Module2.hpp
...
```

Some libraries may contain additional files, such as configuration files, examples, or tests, which are also organized within their respective subdirectories. To simply utilize each Module, you can include the "main" header file for each respective Module.

## 1. Hardware Abstraction Layer (HAL)

### 1.1. Overview

The Hardware Abstraction Layer (HAL) provides a platform-independent interface to abstract specific calls, in particular, Arduino specific calls for this project. This enables code portability and easier testing on different platforms. It automatically detects your platform at compile-time and initializes the correct implementation without any manual setup required. For each important part of a system (e.g., GPIO, I2C, Network, Storage), there is a corresponding HAL module.
The reason for using several HAL modules instead of a single monolithic HAL is to promote modularity and separation of concerns. Each HAL module focuses on a specific aspect of hardware interaction, making it easier to maintain, extend, and test individual components without affecting the entire system.

### 1.2. Details

Each platform-specific implementation inherits from this base class which is purely virtual and provides concrete implementations for the defined methods, by being a singleton. This is controlled through polymorphism, and by using a HAL Manager to handle the selection of the appropriate HAL implementation at runtime. For this reason, each HAL module consists of:

- An abstract purely virtual base class defining the common interface (e.g., `HALGeneral`, `HAL12C`, `HALNetwork`, `HALStorage`).
- Platform-specific implementations inheriting from the base class (e.g., `HALGeneralArduino`, `HALGeneralVirtual`).

The HAL automatically detects your platform at compile-time and initializes the correct implementation. This happens inside a `.cpp` (as an example, `HALGeneral.cpp`). As a result, in most cases, you do not need to do any manual initialization. Further platforms can be added by creating new classes that implements an abstract HAL interface, by then implementing platform-specific behavior.

This approach will yield a consistent interface across platforms, with the following structure. Note that the following HAL modules will be alongside other modules inside `lib/`. For this reason, we recommend using "HAL_name_of_module" as folder naming convention (so the folder names start with "HAL_"). For example, the "G" suffix in `HALGeneral` stands for "General", as to distinguish it from other HAL modules. The following is anjn example of the HAL modules structure:

```text
lib/
├── HALGeneral/       (Basic I/O - GPIO, analog, time, serial)
│   ├── HALGeneral.cpp
│   ├── HALGeneral.hpp
│   ├── HALGeneralArduino.hpp
│   └── HALGeneralVirtual.hpp
├── HAL12C/           (I2C communication)
│   ├── HAL12C.cpp
│   ├── HAL12C.hpp
│   ├── ArduinoPlatform12C.hpp
│   └── VirtualPlatform12C.hpp
├── HALNetwork/       (WiFi/Bluetooth)
├── HALStorage/       (File system + EEPROM)
└── HALInterrupt/     (Interrupt handling)
...
```

## 2. Devices

### 2.1. Overview

The devices are the actual hardware components that the PLC interacts with, such as sensors, actuators, communication modules, etc. Each device has its own driver that implements the necessary functionality to interface with the hardware. The device drivers utilize the HAL to perform hardware-specific operations, allowing for a clean separation between hardware abstraction and device logic.
