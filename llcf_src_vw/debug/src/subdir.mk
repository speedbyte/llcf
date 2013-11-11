################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/af_can.c \
../src/bcm.c \
../src/mcnet.c \
../src/proc.c \
../src/raw.c \
../src/tp16.c \
../src/tp20.c \
../src/tp_gen.c \
../src/vcan.c 

OBJS += \
./src/af_can.o \
./src/bcm.o \
./src/mcnet.o \
./src/proc.o \
./src/raw.o \
./src/tp16.o \
./src/tp20.o \
./src/tp_gen.o \
./src/vcan.o 

C_DEPS += \
./src/af_can.d \
./src/bcm.d \
./src/mcnet.d \
./src/proc.d \
./src/raw.d \
./src/tp16.d \
./src/tp20.d \
./src/tp_gen.d \
./src/vcan.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


