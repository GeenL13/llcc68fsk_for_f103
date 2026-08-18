################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../hardware/llcc68/llcc68.c \
../hardware/llcc68/llcc68_driver_version.c \
../hardware/llcc68/llcc68_hal.c 

OBJS += \
./hardware/llcc68/llcc68.o \
./hardware/llcc68/llcc68_driver_version.o \
./hardware/llcc68/llcc68_hal.o 

C_DEPS += \
./hardware/llcc68/llcc68.d \
./hardware/llcc68/llcc68_driver_version.d \
./hardware/llcc68/llcc68_hal.d 


# Each subdirectory must supply rules for building sources it contributes
hardware/llcc68/%.o hardware/llcc68/%.su hardware/llcc68/%.cyclo: ../hardware/llcc68/%.c hardware/llcc68/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/lianz/Desktop/datalog for car/llcc68fsk_for_f103/app" -I"C:/Users/lianz/Desktop/datalog for car/llcc68fsk_for_f103/hardware" -I"C:/Users/lianz/Desktop/datalog for car/llcc68fsk_for_f103/hardware/llcc68" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-hardware-2f-llcc68

clean-hardware-2f-llcc68:
	-$(RM) ./hardware/llcc68/llcc68.cyclo ./hardware/llcc68/llcc68.d ./hardware/llcc68/llcc68.o ./hardware/llcc68/llcc68.su ./hardware/llcc68/llcc68_driver_version.cyclo ./hardware/llcc68/llcc68_driver_version.d ./hardware/llcc68/llcc68_driver_version.o ./hardware/llcc68/llcc68_driver_version.su ./hardware/llcc68/llcc68_hal.cyclo ./hardware/llcc68/llcc68_hal.d ./hardware/llcc68/llcc68_hal.o ./hardware/llcc68/llcc68_hal.su

.PHONY: clean-hardware-2f-llcc68

