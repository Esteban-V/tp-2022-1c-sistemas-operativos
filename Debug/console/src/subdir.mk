################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../console/src/console.c \
../console/src/utils.c 

C_DEPS += \
./console/src/console.d \
./console/src/utils.d 

OBJS += \
./console/src/console.o \
./console/src/utils.o 


# Each subdirectory must supply rules for building sources it contributes
console/src/%.o: ../console/src/%.c console/src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-console-2f-src

clean-console-2f-src:
	-$(RM) ./console/src/console.d ./console/src/console.o ./console/src/utils.d ./console/src/utils.o

.PHONY: clean-console-2f-src

