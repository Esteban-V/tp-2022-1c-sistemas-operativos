################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/console.c \
../src/utils.c 

OBJS += \
./src/console.o \
./src/utils.o 

C_DEPS += \
./src/console.d \
./src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
<<<<<<< HEAD
	gcc -I"/home/utnso/tp-2022-1c-grupito/shared/include" -I"/home/utnso/tp-2022-1c-grupito/console/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
=======
	gcc -I"/home/utn-so/Desktop/so/tp-2022-1c-grupito/shared/include" -I"/home/utn-so/Desktop/so/tp-2022-1c-grupito/console/include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
>>>>>>> 53ecdd0859ff56386b9fd1a8548efc8492f86f0a
	@echo 'Finished building: $<'
	@echo ' '


