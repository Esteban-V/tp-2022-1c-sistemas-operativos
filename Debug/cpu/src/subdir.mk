################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../cpu/src/cpu.c \
../cpu/src/tlb.c 

OBJS += \
./cpu/src/cpu.o \
./cpu/src/tlb.o 

C_DEPS += \
./cpu/src/cpu.d \
./cpu/src/tlb.d 


# Each subdirectory must supply rules for building sources it contributes
cpu/src/%.o: ../cpu/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


