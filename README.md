# STM32WL55CCU6 projects

This repository contains code implementations for STM32WL55CCU6 microcontroller based projects. Up to date (10/12/2025) it contains two projects, each in a different repo branch.

# Repository Structure


## **"CO2_CozirLP5000_STM32WL55CCU6" branch**
- This project, better described down below, is a code implementation for a CO2 measurement node, with the COZIR LP 5000 sensor.


## **"AT_Slave_STM32WL55CCU6" branch**
- This project, is a code implementation following the Nucleo WL55JC1 AT Slave example, which was used to test the RF matching network.


# CO2 measurement node, based on STM32WL55CCU6 custom PCB and COZIR LP 5000

## Hardware connections

- The COZIR LP 5000 communicates with the STM32WL55CCU6 microcontroller through an UART interface (here UART 1, 9600 Baud).
- The microcontroller is also using UART2 to communicate with the user.
- To have a better understanding of the complete hardware configuration, refer to the wiki page, where the PCB is better described.

## Firmware

- The firmware is based on the LoRaWAN_End_Node skeleton, offered in the STM32 CubeWL package.
- Its aim is to be an user friendly code, which can allow an user to easily interface with the board and with the sensor.
- Through UART2 peripheral communication an user can firstly select between two operating modes: manual and automatic (the default one).
- Please refer to the Wiki page to see the full command table, which a user can send to the board to configure it properly.

### Automatic Mode

- The board is programmed to automatically manage CO2 acquisitions and transmissions.
- Specifically, the board is programmed to alternate sleep and active phases.
- When awake, the system puts the sensor in Mode 2 (refer to Wiki to see the sensor description). It acquires n CO2 samples, averages them, and stores the average in a buffer. If the buffer is full (according to what the user has set), it is transmitted through LoRaWAN. By default: n = 10, buffer size = 5.
- When in sleep, the microcontroller is put in Stop2 mode, while the sensor in Mode 0 (= sleep). No readings are performed, but the sensor is maintained powered on to keep measurement accuracy. The sleep phase duration can be set by the user (default is Ts = 30 s).

## Key Features

- UART interrupt-based reception from CO2 sensor and PC terminal  
- Buffering and averaging of CO2 values over configurable number of samples  
- Dynamic switching between manual and auto data acquisition modes  
- Switching between LoRaWAN and UART for data output  
- LoRaWAN stack integration with join, send, receive callbacks and adaptive transmission timer  
- NVM support for LoRaWAN context storage and restoration  
- Power management hooks with low-power mode enabling/disabling  

## Directory Structure

- `lora_app.c` — Application source file with main logic and callbacks  
- `platform.h`, `sys_app.h` — Board and system abstraction  
- `lora_info.h`, `LmHandler.h` — LoRaWAN middleware and stack  
- `adc_if.h` — ADC Interface (for possible sensor extensions)  
- `flash_if.h` — Flash memory interface for NVM  
- `usart.h` — UART serial interface  

## Architecture and Flow

### Initialization

- `LoRaWAN_Init()`  
  - Initializes flash, LoRaWAN stack, timers, UART callbacks, and modes  
  - Registers tasks with the sequencer (`UTIL_SEQ`)  
  - Starts Join procedure and periodic TX timer (if using `TX_ON_TIMER` event)  

### UART Communication

- **UART1:** Sensor interface  
  - Interrupt-driven RX  
  - Two operation modes:  
    - Manual: interpret and respond to sensor strings and commands  
    - Automatic: read periodic sensor data, buffer multiple readings, average, and store for LoRa send  
- **UART2:** PC terminal interface  
  - Command parser for mode switching (`selection`), transmission mode (`lora`), and reading parameters  
  - Acknowledges user commands and prints UART/LoRa transmission info  

### Sensor Reading and Data Handling

- Task `readCO2` sends `"Z\r\n"` command to sensor to request filtered CO2 concentration  
- 10 readings accumulated in `buf_co2[]`, averaged, and stored in `AppDataBuffer`  
- Automatic transmissions sent after buffer is full or by timer events  

### LoRaWAN Events and Callbacks

- `SendTxData()` sends collected application buffer via LoRaWAN  
- `OnTxTimerEvent()` timer callback triggers periodic sensor reads/transmissions  
- Callbacks handle join status, confirmed/unconfirmed TX, RX downlink frames, beacon status, class change, context storage/restoration  

### Application Data Structure

- `AppDataBuffer[]` holds CO2 average samples formatted as 2-byte integers for LoRa payload  
- Buffer length and data transmission controlled dynamically based on modes and user input  

## Configurable Parameters

| Parameter         | Description                            | Default Value               |
|-------------------|------------------------------------|-----------------------------|
| `readings`        | Number of measurements averaged      | 10                          |
| `readingInterval` | Interval between sensor reads (ms)   | 500                         |
| `TxPeriodicity`   | Periodicity of LoRa transmissions (ms) | Defined by `APP_TX_DUTYCYCLE` |
| `mode`            | Output mode (filtered/unfiltered)    | 2                           |
| `selection`       | Operation mode (manual=1, auto=2)    | 2                           |
| `lora`            | Transmission mode (LoRa=1, UART=0)   | 1                           |

## Usage Notes

- Send commands on UART2 console to toggle modes:  
  - `'c'` to switch between manual and automatic reading modes  
  - `'l'` to toggle between LoRaWAN and UART transmission  
  - `'tNNN'` to set transmission periodicity (e.g., `t10` for 10 seconds)  
  - `'nNNN'` to set number of samples averaged per cycle  
- UART1 receives sensor data line-by-line and processes accordingly  
- Buffer of averaged readings transmits via LoRaWAN when full or can be printed on UART  









## Contributions
Feel free to contribute by improving the existing code or adding new features. Open an issue or submit a pull request if you have any suggestions.

## License
This project is licensed under the MIT License. See the LICENSE file for details.
