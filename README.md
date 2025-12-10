# STM32WL55CCU6 Projects

This repository contains code implementations for STM32WL55CCU6 microcontroller based projects.  
As of 10/12/2025, it contains two projects, each in a different repository branch.

---

## Repository Structure

### **"CO2_CozirLP5000_STM32WL55CCU6" branch**

- This project, detailed below, is a code implementation for a CO2 measurement node using the COZIR LP 5000 sensor.  

### **"AT_Slave_STM32WL55CCU6" branch**

- This project follows the Nucleo WL55JC1 AT Slave example, and was used to test the RF matching network.

---

# CO2 Measurement Node on STM32WL55CCU6 with COZIR LP 5000

## Hardware Connections

- The COZIR LP 5000 sensor communicates with the STM32WL55CCU6 microcontroller via **UART1** at 9600 baud.  
- The microcontroller uses **UART2** to communicate with the user for command input and status output.  
- For full hardware details, refer to the project wiki page describing the custom PCB.

## Firmware Overview

- Based on the STM32 CubeWL LoRaWAN End Node skeleton, extended to interface with the COZIR LP 5000 sensor.  
- Designed for user-friendly interaction through UART2 for configuration and data monitoring.

---

## Operation Modes

### Manual Mode

- The user interacts with the board via UART2 commands.  
- The board waits for sensor data lines on UART1 and prints them directly or interprets commands.  
- Users can query sensor values or configure parameters on demand.  
- Sensor data is **not automatically averaged or buffered** in this mode.

### Automatic Mode (Default)

- The board autonomously manages data acquisition and transmission cycles.  
- The microcontroller wakes periodically, setting the sensor to **Mode 2** (refer to sensor documentation).  
- It acquires *n* CO2 samples (default 10), averages them, and stores the averages in an internal buffer.  
- Once the buffer fills (default size 5 averages), it transmits the data via LoRaWAN (or UART if selected).  
- During sleep phases, the microcontroller enters **Stop2 mode** and the sensor into **Mode 0 (sleep)** while keeping power supplied for accuracy.  
- Sleep duration (*Ts*) is configurable by the user; default is 30 seconds.

---

## Key Features

- UART interrupt-driven reception from both CO2 sensor (UART1) and PC terminal (UART2).  
- Buffering and averaging configurable number of CO2 readings.  
- Seamless switching between manual and automatic modes.  
- Dual transmission options: LoRaWAN or UART output.  
- Full LoRaWAN stack integration (join, Tx/Rx callbacks, adaptive timers).  
- Non-volatile memory (NVM) support for LoRaWAN context.  
- Power management with configurable low-power modes.

---

## Important Configuration Files

| File                                                      | Description                                                                                           |
|-----------------------------------------------------------|---------------------------------------------------------------------------------------------------|
| `Custom_board_LoRa_WL55CCU6/LoRaWAN/App/lora_app.c`       | Core application logic and callbacks handling sensor data, UART, and LoRaWAN communication.       |
| `Custom_board_LoRa_WL55CCU6/LoRaWAN/App/se-identity.h`    | LoRaWAN keys (recommended to modify through CubeMX `.ioc` file, not here).                         |
| `Custom_board_LoRa_WL55CCU6/Core/Src/main.c`               | Main firmware entry point.                                                                          |
| `Custom_board_LoRa_WL55CCU6/Core/Src/stm32_lpm_if.c`       | Low power mode management functions.                                                               |
| `Custom_board_LoRa_WL55CCU6/Core/Inc/sys_conf.h`           | Initial configuration: disables Stop2 mode at startup for UART config, later re-enabled.           |
| `Custom_board_LoRa_WL55CCU6/Core/Inc/utilities_def.h`      | Task ID declarations for sequencer-controlled tasks.                                              |
| `Custom_board_LoRa_WL55CCU6/Drivers/BSP/STM32WLxx_Nucleo/` | Radio configuration files — manage RF switch pins, LP/HP transmission power, and TCXO enablement. |

---

## Important Reminders

- **CubeMX Regeneration:**  
  Remove the UART Rx Callback from `Custom_board_LoRa_WL55CCU6/Core/Src/usart_if.c` because it is implemented in `lora_app.c`.  

- **Transmission Control:**  
  After regenerating code, remember to **comment out or remove in `OnTxTimerEvent`** the line that triggers transmission, since the CO2 reading routine manages transmissions.

---

## UART Command Interface

User commands are sent through **UART2**, terminated by `\r\n`. Key commands include:

| Command               | Description                                                            | Notes                                        |
|-----------------------|------------------------------------------------------------------------|----------------------------------------------|
| `c\r\n`               | Toggle between manual and automatic operation modes                   | Automatic mode is default                     |
| `l\r\n`               | Toggle data output between LoRaWAN (default) and UART transmission    |                                              |
| `t<number>\r\n`       | Set transmission periodicity in seconds                               | For example, `t10\r\n` sets 10 seconds        |
| `n<number>\r\n`       | Set number of sensor samples averaged before storing or transmitting  | For example, `n5\r\n` sets average of 5 samples |
| `K <mode_number>\r\n` | Set sensor sensor output mode (in manual mode)                        | See sensor datasheet; 0,1,2 available         |
| `Z\r\n` or sensor data lines | Sensor responses, automatically requested in auto mode | Sensor sends filtered CO2 concentration       |

---

## Configurable Parameters

| Parameter         | Description                              | Default Value                 |
|-------------------|--------------------------------------|-------------------------------|
| `readings`        | Number of CO2 samples to average per cycle | 10                            |
| `readingInterval` | Interval between sensor readings (milliseconds) | 500                           |
| `TxPeriodicity`   | Periodicity between LoRa transmissions (milliseconds) | Defined by `APP_TX_DUTYCYCLE` |
| `mode`            | Sensor read mode (filtered/unfiltered output) | 2                             |
| `selection`       | Operation mode: manual (1) or automatic (2) | 2                             |
| `lora`            | Transmission medium: LoRaWAN (1) or UART (0) | 1                             |

---

## Usage Notes

- Use UART2 to send commands to configure device behavior and inspect sensor data.  
- UART1 automatically receives sensor data lines according to mode.  
- In **automatic mode**, sensor readings are acquired periodically, averaged, buffered, and transmitted without user intervention.  
- In **manual mode**, user sends commands and receives immediate sensor output without averaging or buffering.

---

## Contributions

Feel free to contribute by improving the existing code or adding new features. Submit issues or pull requests for suggestions.

---

## License

This project is licensed under the MIT License. See the LICENSE file for details.
