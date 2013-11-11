################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/drivers/mscan/mscan_hw.c \
../src/drivers/mscan/mscan_net.c \
../src/drivers/mscan/mscan_proc.c 

OBJS += \
./src/drivers/mscan/mscan_hw.o \
./src/drivers/mscan/mscan_net.o \
./src/drivers/mscan/mscan_proc.o 

C_DEPS += \
./src/drivers/mscan/mscan_hw.d \
./src/drivers/mscan/mscan_net.d \
./src/drivers/mscan/mscan_proc.d 


# Each subdirectory must supply rules for building sources it contributes
src/drivers/mscan/%.o: ../src/drivers/mscan/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


