################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/test/can-sniffer.c \
../src/test/candump.c \
../src/test/canecho.c \
../src/test/mcnet-sniffer.c \
../src/test/mcnet-vit-emu.c \
../src/test/pq35speed.c \
../src/test/tp16-client.c \
../src/test/tp16-server.c \
../src/test/tp20-client-mdi.c \
../src/test/tp20-client.c \
../src/test/tp20-server.c \
../src/test/tp20-sniffer.c \
../src/test/tpdump.c \
../src/test/tst-bcm-cycle.c \
../src/test/tst-bcm-filter.c \
../src/test/tst-bcm-rtr.c \
../src/test/tst-bcm-single.c \
../src/test/tst-bcm-throttle.c \
../src/test/tst-bcm-tx_read.c \
../src/test/tst-proc.c \
../src/test/tst-raw-filter.c \
../src/test/tst-raw-sendto-SingStream.c \
../src/test/tst-raw-sendto.c \
../src/test/tst-raw.c 

OBJS += \
./src/test/can-sniffer.o \
./src/test/candump.o \
./src/test/canecho.o \
./src/test/mcnet-sniffer.o \
./src/test/mcnet-vit-emu.o \
./src/test/pq35speed.o \
./src/test/tp16-client.o \
./src/test/tp16-server.o \
./src/test/tp20-client-mdi.o \
./src/test/tp20-client.o \
./src/test/tp20-server.o \
./src/test/tp20-sniffer.o \
./src/test/tpdump.o \
./src/test/tst-bcm-cycle.o \
./src/test/tst-bcm-filter.o \
./src/test/tst-bcm-rtr.o \
./src/test/tst-bcm-single.o \
./src/test/tst-bcm-throttle.o \
./src/test/tst-bcm-tx_read.o \
./src/test/tst-proc.o \
./src/test/tst-raw-filter.o \
./src/test/tst-raw-sendto-SingStream.o \
./src/test/tst-raw-sendto.o \
./src/test/tst-raw.o 

C_DEPS += \
./src/test/can-sniffer.d \
./src/test/candump.d \
./src/test/canecho.d \
./src/test/mcnet-sniffer.d \
./src/test/mcnet-vit-emu.d \
./src/test/pq35speed.d \
./src/test/tp16-client.d \
./src/test/tp16-server.d \
./src/test/tp20-client-mdi.d \
./src/test/tp20-client.d \
./src/test/tp20-server.d \
./src/test/tp20-sniffer.d \
./src/test/tpdump.d \
./src/test/tst-bcm-cycle.d \
./src/test/tst-bcm-filter.d \
./src/test/tst-bcm-rtr.d \
./src/test/tst-bcm-single.d \
./src/test/tst-bcm-throttle.d \
./src/test/tst-bcm-tx_read.d \
./src/test/tst-proc.d \
./src/test/tst-raw-filter.d \
./src/test/tst-raw-sendto-SingStream.d \
./src/test/tst-raw-sendto.d \
./src/test/tst-raw.d 


# Each subdirectory must supply rules for building sources it contributes
src/test/%.o: ../src/test/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


