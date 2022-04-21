################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../cpu/src/cpu.c 

C_DEPS += \
./cpu/src/cpu.d 

OBJS += \
./cpu/src/cpu.o 


# Each subdirectory must supply rules for building sources it contributes
cpu/src/%.o: ../cpu/src/%.c cpu/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-cpu-2f-src

clean-cpu-2f-src:
	-$(RM) ./cpu/src/cpu.d ./cpu/src/cpu.o

.PHONY: clean-cpu-2f-src

