################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include shared/src/subdir.mk
-include shared/Debug/src/subdir.mk
-include memory/src/subdir.mk
-include kernel/src/subdir.mk
-include kernel/Debug/src/subdir.mk
-include cpu/src/subdir.mk
-include console/src/subdir.mk
-include console/Debug/src/subdir.mk
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
all: kiss

# Tool invocations
kiss: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc  -o "kiss" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) kiss
	-@echo ' '

.PHONY: all clean dependents

-include ../makefile.targets
