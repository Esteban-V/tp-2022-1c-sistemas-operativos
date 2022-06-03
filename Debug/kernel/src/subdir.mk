################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../kernel/src/kernel.c \
../kernel/src/p_queue.c \
../kernel/src/utils.c 

C_DEPS += \
./kernel/src/kernel.d \
./kernel/src/p_queue.d \
./kernel/src/utils.d 

OBJS += \
./kernel/src/kernel.o \
./kernel/src/p_queue.o \
./kernel/src/utils.o 


# Each subdirectory must supply rules for building sources it contributes
kernel/src/%.o: ../kernel/src/%.c kernel/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-kernel-2f-src

clean-kernel-2f-src:
	-$(RM) ./kernel/src/kernel.d ./kernel/src/kernel.o ./kernel/src/p_queue.d ./kernel/src/p_queue.o ./kernel/src/utils.d ./kernel/src/utils.o

.PHONY: clean-kernel-2f-src

