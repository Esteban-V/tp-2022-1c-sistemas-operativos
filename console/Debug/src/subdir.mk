################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/console.c \
../src/utils.c 

C_DEPS += \
./src/console.d \
./src/utils.d 

OBJS += \
./src/console.o \
./src/utils.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/esteban/Desktop/tp/tp-2022-1c-grupito/shared/include" -I"/home/esteban/Desktop/tp/tp-2022-1c-grupito/console/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/console.d ./src/console.o ./src/utils.d ./src/utils.o

.PHONY: clean-src

