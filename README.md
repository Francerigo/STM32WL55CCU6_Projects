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











## Contributions
Feel free to contribute by improving the existing code or adding new features. Open an issue or submit a pull request if you have any suggestions.

## License
This project is licensed under the MIT License. See the LICENSE file for details.
