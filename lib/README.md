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

## 1. Firmware Implementation Details

This section summarizes how the firmware is assembled and executed at runtime. The core idea is simple: `App` boots and wires the system, `StateMachine` coordinates decisions, and device modules execute actions — synchronous GPIO devices inline on the caller's task, the I2C thermocouple on its own FreeRTOS task.
Since the project utilizes Arduino as the Framework (due to the libraries needed for utilizing the ESP-32 PLC's) which uses the ESP-IDF, the `vTaskStartScheduler()` function is run automatically after setting up the device and does not need to be run in the main application.

The runtime is organized around two central components:

- `App` as the main entry point and startup orchestrator.
- `StateMachine` as the state controller and mediator between modules.

### 1.1. Firmware Architecture

The firmware architecture can be grouped into three layers that clarify ownership and simplify maintenance.

1. **High Level Logic (`App`, `StateMachine`)**
   - `App` acts as the entry point and orchestration root: startup order and task startup.
   - `StateMachine` defines system behavior: state transitions, decisions, and command mediation.

2. **Middle Level Logic (Device Modules)**
   - `Communications` connects protocol inputs to this layer by parsing serial/ethernet messages and forwarding requests.
   - Modules implement domain-level actuator and sensor behavior.
   - GPIO modules (`SyncDevice`) execute accepted commands inline on the caller's task; the I2C `Thermocouple` (`AsyncDevice`) owns a queue/task.
   - Modules report executed commands back to `StateMachine` for confirmation; autonomous control-loop and safe-output actuations are enforced without a report.

3. **System Utilities (`FreeRTOS`, `HAL`)**
   - `FreeRTOS` provides scheduler, task model, delays, and queue primitives used by upper layers.
   - `HAL` abstracts platform-specific I/O (GPIO, timing, logging, etc.) behind stable interfaces.

### 1.2. FreeRTOS

FreeRTOS is used to make the firmware responsive, modular, and safe under concurrent I/O and control workloads. Instead of running all logic in a single loop, the system assigns independent responsibilities to dedicated tasks and communicates through queues. It allows

- Only the I2C `Thermocouple` owns its own queue and task; GPIO devices actuate synchronously inline.
- Control loops (`StateMachine` and `App`) run periodically and concurrently by utilizing the FreeRTOS scheduler.
- Producers enqueue commands/events; consumers process asynchronously.

#### 1.2.1. Core Responsibilities

The ESP32 is dual-core, in particular, the main target device we are using (PLC 38AR) is dual-core. Tasks are explicitly pinned to cores to isolate bus traffic:

| Core | Tasks | Rationale |
|------|-------|-----------|
| **0 (PRO\_CPU)** | `App`, `StateMachine`, inline `SyncDevice` actuation | Ethernet polling drives SPI/APB traffic. Keeping these together avoids cross-core coordination overhead. |
| **1 (APP\_CPU)** | `Thermocouple` task (I2C only) | Blocking I2C transactions on a shared bus. Isolating from Core 0 prevents SPI APB bursts from time-slicing into I2C transactions, which can corrupt I2C timing on the ESP32. GPIO devices (`SyncDevice`) actuate inline on Core 0. |

The `Thermocouple` task is intentionally set to a high priority (see `Config.hpp`) so it is never preempted mid-I2C-transaction by a same-core peer.

### 1.3. Startup and Runtime Flow

At boot, `src/main.cpp` calls `App::instance().begin()`. From there, `App` performs the high-level initialization sequence:

1. Initializes core services and HAL-backed utilities.
2. Initializes device modules.
3. Initializes the `StateMachine`.
4. Starts module tasks/queues and the `StateMachine` task.
5. Starts networking/local server and communications callbacks.
6. Starts the `App` task, which periodically executes the application tick to poll for incoming external messages.

### 1.4. StateMachine as "Mediator"

`StateMachine` plays a dual role:

- **Finite State Machine (FSM):** manages states and transitions.
- **Mediator:** centralizes coordination between communications and actuator modules.

Commands, events and actions are enqueued into the `StateMachine` queues. The state machine task consumes these queues, validates behavior against current state, and dispatches accepted commands to the corresponding module queue/task. The per-state behavior now lives in dedicated state modules, so `StateMachine` acts as a thin coordinator while each state owns its own transition logic and entry behavior. This keeps module-to-module coupling low: modules do not need to call each other directly.

### 1.5. Communications Interaction

This separation is what allows protocol handling to evolve without changing actuator logic, and vice versa.
`Communications` parses incoming commands/events (serial/ethernet) and forwards them to `StateMachine` through basic enqueue methods. It does not execute actuator logic directly.

### 1.6. Hardware Abstraction Layer (HAL)

#### 1.6.1. Overview

The Hardware Abstraction Layer (HAL) provides a platform-independent interface to abstract specific calls, in particular, Arduino specific calls for this project. This enables code portability and easier testing on different platforms. It automatically detects your platform at compile-time and initializes the correct implementation without any manual setup required. For each important part of a system (e.g., GPIO, I2C, Network, Storage), there is a corresponding HAL module.
The reason for using several HAL modules instead of a single monolithic HAL is to promote modularity and separation of concerns. Each HAL module focuses on a specific aspect of hardware interaction, making it easier to maintain, extend, and test individual components without affecting the entire system.

#### 1.6.2. Details

Each platform-specific implementation inherits from this base class which is purely virtual and provides concrete implementations for the defined methods, by being a singleton. This is controlled through polymorphism, and by using a HAL Manager to handle the selection of the appropriate HAL implementation at runtime. For this reason, each HAL module consists of:

- An abstract purely virtual base class defining the common interface (e.g., `HALGeneral`, `HAL12C`, `HALNetwork`, `HALStorage`).
- Platform-specific implementations inheriting from the base class (e.g., `HALGeneralArduino`).

The HAL automatically detects your platform at compile-time and initializes the correct implementation. This happens inside a `.cpp` (as an example, `HALGeneral.cpp`). As a result, in most cases, you do not need to do any manual initialization. Further platforms can be added by creating new classes that implements an abstract HAL interface, by then implementing platform-specific behavior.

This approach will yield a consistent interface across platforms, with the following structure. Note that the following HAL modules will be alongside other modules inside `lib/`. For this reason, we recommend using "HAL_name_of_module" as folder naming convention (so the folder names start with "HAL_"). For example, the "G" suffix in `HALGeneral` stands for "General", as to distinguish it from other HAL modules. The following is anjn example of the HAL modules structure:

```text
lib/
├── HALGeneral/       (Basic I/O - GPIO, analog, time, serial)
│   ├── HALGeneral.cpp
│   ├── HALGeneral.hpp
│   └── HALGeneralArduino.hpp
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

## 3. Runtime Configuration

All tunable firmware parameters are centralised in a single header:

```
lib/Definitions/Config.hpp
```

Constants are grouped under the `config::` namespace. The file is the **only place** that should be edited when adjusting timing, task scheduling, or network settings.

### 3.1. Encaspulation

| Group | Examples |
|-------|---------|
| **Task Periods** | `kAppTaskPeriodMs`, `kStateMachineTaskPeriodMs` |
| **Task Priorities** | `kAppTaskPriority`, `kStateMachineTaskPriority`, `kThermocoupleTaskPriority` |
| **Core Affinity** | `kSpiCoreId` (Core 0), `kI2cCoreId` (Core 1) |
| **Stack Sizes** | `kAppTaskStackSize`, `kThermocoupleTaskStackSize`, … |
| **Queue Depths** | `kStateMachineQueueLength`, `kDeviceQueueLength` |
| **Timeouts** | `kCommandFeedbackTimeoutMs`, `kCommsQueueTimeoutMs`, `kSpiMutexPollTimeoutMs`, … |
| **SM Tick Limits** | `kMaxEventsPerTick`, `kMaxCommandsPerTick`, `kMaxResponsesPerTick` |
| **Network** | `kEthernetPort` *(pending)*, `kSerialBaudRate` *(pending)* |

On the other hand, the following are not included here, since the configuration parameters are meant to be modified at any firmware release.

- **Protocol definitions** (SCPI command strings, state names, response maps) → `Definitions.hpp`
- **Hardware pin assignments and I2C addresses** → `PLC38AR.hpp` / `PLC19R.hpp`
- **Task name strings** — debug identifiers that belong next to the task they describe
- **`portMAX_DELAY` watchdog ticks** — a design decision ("block until work arrives"), not a tunable parameter
