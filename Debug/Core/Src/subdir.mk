################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/bmi270_port.c \
../Core/Src/bms_task.c \
../Core/Src/gps_task.c \
../Core/Src/imu_task.c \
../Core/Src/main.c \
../Core/Src/nand_spi_port.c \
../Core/Src/nmea_parse.c \
../Core/Src/stm32u3xx_hal_msp.c \
../Core/Src/stm32u3xx_it.c \
../Core/Src/storage_task.c \
../Core/Src/syscalls.c \
../Core/Src/system_control.c \
../Core/Src/system_stm32u3xx.c \
../Core/Src/temp_task.c \
../Core/Src/usb_cdc_task.c 

OBJS += \
./Core/Src/bmi270_port.o \
./Core/Src/bms_task.o \
./Core/Src/gps_task.o \
./Core/Src/imu_task.o \
./Core/Src/main.o \
./Core/Src/nand_spi_port.o \
./Core/Src/nmea_parse.o \
./Core/Src/stm32u3xx_hal_msp.o \
./Core/Src/stm32u3xx_it.o \
./Core/Src/storage_task.o \
./Core/Src/syscalls.o \
./Core/Src/system_control.o \
./Core/Src/system_stm32u3xx.o \
./Core/Src/temp_task.o \
./Core/Src/usb_cdc_task.o 

C_DEPS += \
./Core/Src/bmi270_port.d \
./Core/Src/bms_task.d \
./Core/Src/gps_task.d \
./Core/Src/imu_task.d \
./Core/Src/main.d \
./Core/Src/nand_spi_port.d \
./Core/Src/nmea_parse.d \
./Core/Src/stm32u3xx_hal_msp.d \
./Core/Src/stm32u3xx_it.d \
./Core/Src/storage_task.d \
./Core/Src/syscalls.d \
./Core/Src/system_control.d \
./Core/Src/system_stm32u3xx.d \
./Core/Src/temp_task.d \
./Core/Src/usb_cdc_task.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32U385xx -DUX_INCLUDE_USER_DEFINE_FILE -c -I../Core/Inc -I../Drivers/STM32U3xx_HAL_Driver/Inc -I../Drivers/STM32U3xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32U3xx/Include -I../Drivers/CMSIS/Include -I../USBX/App -I../USBX/Target -I../Middlewares/ST/usbx/common/core/inc -I../Middlewares/ST/usbx/ports/generic/inc -I../Middlewares/ST/usbx/common/usbx_stm32_device_controllers -I../Middlewares/ST/usbx/common/usbx_device_classes/inc -I../Drivers/BMI270 -I../Drivers/NAND_M79A -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/bmi270_port.cyclo ./Core/Src/bmi270_port.d ./Core/Src/bmi270_port.o ./Core/Src/bmi270_port.su ./Core/Src/bms_task.cyclo ./Core/Src/bms_task.d ./Core/Src/bms_task.o ./Core/Src/bms_task.su ./Core/Src/gps_task.cyclo ./Core/Src/gps_task.d ./Core/Src/gps_task.o ./Core/Src/gps_task.su ./Core/Src/imu_task.cyclo ./Core/Src/imu_task.d ./Core/Src/imu_task.o ./Core/Src/imu_task.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/nand_spi_port.cyclo ./Core/Src/nand_spi_port.d ./Core/Src/nand_spi_port.o ./Core/Src/nand_spi_port.su ./Core/Src/nmea_parse.cyclo ./Core/Src/nmea_parse.d ./Core/Src/nmea_parse.o ./Core/Src/nmea_parse.su ./Core/Src/stm32u3xx_hal_msp.cyclo ./Core/Src/stm32u3xx_hal_msp.d ./Core/Src/stm32u3xx_hal_msp.o ./Core/Src/stm32u3xx_hal_msp.su ./Core/Src/stm32u3xx_it.cyclo ./Core/Src/stm32u3xx_it.d ./Core/Src/stm32u3xx_it.o ./Core/Src/stm32u3xx_it.su ./Core/Src/storage_task.cyclo ./Core/Src/storage_task.d ./Core/Src/storage_task.o ./Core/Src/storage_task.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/system_control.cyclo ./Core/Src/system_control.d ./Core/Src/system_control.o ./Core/Src/system_control.su ./Core/Src/system_stm32u3xx.cyclo ./Core/Src/system_stm32u3xx.d ./Core/Src/system_stm32u3xx.o ./Core/Src/system_stm32u3xx.su ./Core/Src/temp_task.cyclo ./Core/Src/temp_task.d ./Core/Src/temp_task.o ./Core/Src/temp_task.su ./Core/Src/usb_cdc_task.cyclo ./Core/Src/usb_cdc_task.d ./Core/Src/usb_cdc_task.o ./Core/Src/usb_cdc_task.su

.PHONY: clean-Core-2f-Src

