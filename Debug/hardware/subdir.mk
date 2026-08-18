################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../hardware/serial.c \
../hardware/tick.c 

OBJS += \
./hardware/serial.o \
./hardware/tick.o 

C_DEPS += \
./hardware/serial.d \
./hardware/tick.d 


# Each subdirectory must supply rules for building sources it contributes
hardware/%.o hardware/%.su hardware/%.cyclo: ../hardware/%.c hardware/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/lianz/Desktop/datalog for car/llcc68fsk_for_f103/app" -I"C:/Users/lianz/Desktop/datalog for car/llcc68fsk_for_f103/hardware" -I"C:/Users/lianz/Desktop/datalog for car/llcc68fsk_for_f103/hardware/llcc68" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-hardware

clean-hardware:
	-$(RM) ./hardware/serial.cyclo ./hardware/serial.d ./hardware/serial.o ./hardware/serial.su ./hardware/tick.cyclo ./hardware/tick.d ./hardware/tick.o ./hardware/tick.su

.PHONY: clean-hardware

