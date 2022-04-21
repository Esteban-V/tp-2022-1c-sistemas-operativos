################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../memory/src/memory.c \
../memory/src/utils.c 

C_DEPS += \
./memory/src/memory.d \
./memory/src/utils.d 

OBJS += \
./memory/src/memory.o \
./memory/src/utils.o 


# Each subdirectory must supply rules for building sources it contributes
memory/src/%.o: ../memory/src/%.c memory/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-memory-2f-src

clean-memory-2f-src:
	-$(RM) ./memory/src/memory.d ./memory/src/memory.o ./memory/src/utils.d ./memory/src/utils.o

.PHONY: clean-memory-2f-src

