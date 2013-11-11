################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/drivers/tricore/twincan.c \
../src/drivers/tricore/twincan_hw.c 

OBJS += \
./src/drivers/tricore/twincan.o \
./src/drivers/tricore/twincan_hw.o 

C_DEPS += \
./src/drivers/tricore/twincan.d \
./src/drivers/tricore/twincan_hw.d 


# Each subdirectory must supply rules for building sources it contributes
src/drivers/tricore/%.o: ../src/drivers/tricore/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


