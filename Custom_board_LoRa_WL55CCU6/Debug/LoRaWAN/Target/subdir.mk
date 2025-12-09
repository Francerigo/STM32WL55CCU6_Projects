################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../LoRaWAN/Target/radio_board_if.c 

OBJS += \
./LoRaWAN/Target/radio_board_if.o 

C_DEPS += \
./LoRaWAN/Target/radio_board_if.d 


# Each subdirectory must supply rules for building sources it contributes
LoRaWAN/Target/%.o LoRaWAN/Target/%.su LoRaWAN/Target/%.cyclo: ../LoRaWAN/Target/%.c LoRaWAN/Target/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WL55xx -c -I../Core/Inc -I../LoRaWAN/App -I../LoRaWAN/Target -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Drivers/STM32WLxx_HAL_Driver/Inc -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Utilities/trace/adv_trace -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Utilities/misc -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Utilities/sequencer -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Utilities/timer -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Utilities/lpm/tiny_lpm -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Middlewares/Third_Party/LoRaWAN/LmHandler/Packages -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Drivers/CMSIS/Device/ST/STM32WLxx/Include -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Middlewares/Third_Party/LoRaWAN/Crypto -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Middlewares/Third_Party/LoRaWAN/Mac/Region -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Middlewares/Third_Party/LoRaWAN/Mac -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Middlewares/Third_Party/LoRaWAN/LmHandler -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Middlewares/Third_Party/LoRaWAN/Utilities -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Middlewares/Third_Party/SubGHz_Phy -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver -IC:/Users/LabMeas/STM32Cube/Repository/STM32Cube_FW_WL_V1.3.1/Drivers/CMSIS/Include -I../Drivers/BSP/STM32WLxx_Nucleo -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-LoRaWAN-2f-Target

clean-LoRaWAN-2f-Target:
	-$(RM) ./LoRaWAN/Target/radio_board_if.cyclo ./LoRaWAN/Target/radio_board_if.d ./LoRaWAN/Target/radio_board_if.o ./LoRaWAN/Target/radio_board_if.su

.PHONY: clean-LoRaWAN-2f-Target

