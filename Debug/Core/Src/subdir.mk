################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/bmi2.c \
../Core/Src/bmi270.c \
../Core/Src/bq25629.c \
../Core/Src/gps.c \
../Core/Src/main.c \
../Core/Src/nand_m79a.c \
../Core/Src/nand_m79a_lld.c \
../Core/Src/nand_spi.c \
../Core/Src/protocol.c \
../Core/Src/stm32u3xx_hal_msp.c \
../Core/Src/stm32u3xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32u3xx.c \
../Core/Src/usbx_device.c \
../Core/Src/wifi_ble.c 

OBJS += \
./Core/Src/bmi2.o \
./Core/Src/bmi270.o \
./Core/Src/bq25629.o \
./Core/Src/gps.o \
./Core/Src/main.o \
./Core/Src/nand_m79a.o \
./Core/Src/nand_m79a_lld.o \
./Core/Src/nand_spi.o \
./Core/Src/protocol.o \
./Core/Src/stm32u3xx_hal_msp.o \
./Core/Src/stm32u3xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32u3xx.o \
./Core/Src/usbx_device.o \
./Core/Src/wifi_ble.o 

C_DEPS += \
./Core/Src/bmi2.d \
./Core/Src/bmi270.d \
./Core/Src/bq25629.d \
./Core/Src/gps.d \
./Core/Src/main.d \
./Core/Src/nand_m79a.d \
./Core/Src/nand_m79a_lld.d \
./Core/Src/nand_spi.d \
./Core/Src/protocol.d \
./Core/Src/stm32u3xx_hal_msp.d \
./Core/Src/stm32u3xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32u3xx.d \
./Core/Src/usbx_device.d \
./Core/Src/wifi_ble.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U385xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Drivers/STM32U3xx_HAL_Driver/Inc -I../Drivers/STM32U3xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U3xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/bmi2.cyclo ./Core/Src/bmi2.d ./Core/Src/bmi2.o ./Core/Src/bmi2.su ./Core/Src/bmi270.cyclo ./Core/Src/bmi270.d ./Core/Src/bmi270.o ./Core/Src/bmi270.su ./Core/Src/bq25629.cyclo ./Core/Src/bq25629.d ./Core/Src/bq25629.o ./Core/Src/bq25629.su ./Core/Src/gps.cyclo ./Core/Src/gps.d ./Core/Src/gps.o ./Core/Src/gps.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/nand_m79a.cyclo ./Core/Src/nand_m79a.d ./Core/Src/nand_m79a.o ./Core/Src/nand_m79a.su ./Core/Src/nand_m79a_lld.cyclo ./Core/Src/nand_m79a_lld.d ./Core/Src/nand_m79a_lld.o ./Core/Src/nand_m79a_lld.su ./Core/Src/nand_spi.cyclo ./Core/Src/nand_spi.d ./Core/Src/nand_spi.o ./Core/Src/nand_spi.su ./Core/Src/protocol.cyclo ./Core/Src/protocol.d ./Core/Src/protocol.o ./Core/Src/protocol.su ./Core/Src/stm32u3xx_hal_msp.cyclo ./Core/Src/stm32u3xx_hal_msp.d ./Core/Src/stm32u3xx_hal_msp.o ./Core/Src/stm32u3xx_hal_msp.su ./Core/Src/stm32u3xx_it.cyclo ./Core/Src/stm32u3xx_it.d ./Core/Src/stm32u3xx_it.o ./Core/Src/stm32u3xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32u3xx.cyclo ./Core/Src/system_stm32u3xx.d ./Core/Src/system_stm32u3xx.o ./Core/Src/system_stm32u3xx.su ./Core/Src/usbx_device.cyclo ./Core/Src/usbx_device.d ./Core/Src/usbx_device.o ./Core/Src/usbx_device.su ./Core/Src/wifi_ble.cyclo ./Core/Src/wifi_ble.d ./Core/Src/wifi_ble.o ./Core/Src/wifi_ble.su

.PHONY: clean-Core-2f-Src

