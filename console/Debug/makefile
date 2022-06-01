################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: console

# Tool invocations
console: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
<<<<<<< HEAD
	gcc -L"/home/utnso/tp-2022-1c-grupito/shared/Debug" -o "console" $(OBJS) $(USER_OBJS) $(LIBS)
=======
	gcc -L"/home/utn-so/Desktop/so/tp-2022-1c-grupito/shared/Debug" -o "console" $(OBJS) $(USER_OBJS) $(LIBS)
>>>>>>> 53ecdd0859ff56386b9fd1a8548efc8492f86f0a
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) console
	-@echo ' '

.PHONY: all clean dependents

-include ../makefile.targets
