################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../memory/src/memory.c \
../memory/src/pageTable.c \
../memory/src/swap.c \
../memory/src/utils.c 

OBJS += \
./memory/src/memory.o \
./memory/src/pageTable.o \
./memory/src/swap.o \
./memory/src/utils.o 

C_DEPS += \
./memory/src/memory.d \
./memory/src/pageTable.d \
./memory/src/swap.d \
./memory/src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
memory/src/%.o: ../memory/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


