################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../app/packet.c \
../app/pool.c \
../app/radio.c 

OBJS += \
./app/packet.o \
./app/pool.o \
./app/radio.o 

C_DEPS += \
./app/packet.d \
./app/pool.d \
./app/radio.d 


# Each subdirectory must supply rules for building sources it contributes
app/%.o app/%.su app/%.cyclo: ../app/%.c app/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/lianz/Desktop/datalog for car/llcc68fsk_for_f103/app" -I"C:/Users/lianz/Desktop/datalog for car/llcc68fsk_for_f103/hardware" -I"C:/Users/lianz/Desktop/datalog for car/llcc68fsk_for_f103/hardware/llcc68" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-app

clean-app:
	-$(RM) ./app/packet.cyclo ./app/packet.d ./app/packet.o ./app/packet.su ./app/pool.cyclo ./app/pool.d ./app/pool.o ./app/pool.su ./app/radio.cyclo ./app/radio.d ./app/radio.o ./app/radio.su

.PHONY: clean-app

