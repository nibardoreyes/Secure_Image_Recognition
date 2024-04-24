################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lcd/LCD_Driver.c \
../lcd/LCD_Hw.c \
../lcd/LCD_Lib.c \
../lcd/font.c \
../lcd/lcd_graphic.c 

O_SRCS += \
../lcd/LCD_Driver.o \
../lcd/LCD_Hw.o \
../lcd/LCD_Lib.o \
../lcd/font.o \
../lcd/lcd_graphic.o \
../lcd/terasic_lib.o 

OBJS += \
./lcd/LCD_Driver.o \
./lcd/LCD_Hw.o \
./lcd/LCD_Lib.o \
./lcd/font.o \
./lcd/lcd_graphic.o 

C_DEPS += \
./lcd/LCD_Driver.d \
./lcd/LCD_Hw.d \
./lcd/LCD_Lib.d \
./lcd/font.d \
./lcd/lcd_graphic.d 


# Each subdirectory must supply rules for building sources it contributes
lcd/%.o: ../lcd/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler 4.8.3 [arm-linux-gnueabihf]'
	arm-linux-gnueabihf-gcc.exe -std=c99 -Dsoc_cv_av -I"C:\Users\Nibardo Reyes\Development Studio Workspace\450final\lcd" -I"C:\intelFPGA\20.1\embedded\ip\altera\hps\altera_hps\hwlib\include\soc_cv_av" -I"C:\intelFPGA\20.1\embedded\ip\altera\hps\altera_hps\hwlib\include" -O0 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


