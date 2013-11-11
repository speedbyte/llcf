################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/drivers/sja1000/isa.c \
../src/drivers/sja1000/proc.c \
../src/drivers/sja1000/sja1000.c \
../src/drivers/sja1000/trajet-gw2.c 

OBJS += \
./src/drivers/sja1000/isa.o \
./src/drivers/sja1000/proc.o \
./src/drivers/sja1000/sja1000.o \
./src/drivers/sja1000/trajet-gw2.o 

C_DEPS += \
./src/drivers/sja1000/isa.d \
./src/drivers/sja1000/proc.d \
./src/drivers/sja1000/sja1000.d \
./src/drivers/sja1000/trajet-gw2.d 


# Each subdirectory must supply rules for building sources it contributes
src/drivers/sja1000/%.o: ../src/drivers/sja1000/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


