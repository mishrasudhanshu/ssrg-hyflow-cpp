################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/util/unitTests/boost_serialization/subdir.mk
-include src/util/unitTests/subdir.mk
-include src/util/parser/subdir.mk
-include src/util/networking/msgConnect/MsgConnect/subdir.mk
-include src/util/networking/msgConnect/subdir.mk
-include src/util/networking/subdir.mk
-include src/util/messages/types/subdir.mk
-include src/util/messages/subdir.mk
-include src/util/logging/Basic/subdir.mk
-include src/util/logging/subdir.mk
-include src/util/concurrent/subdir.mk
-include src/core/exceptions/types/subdir.mk
-include src/core/directory/types/subdir.mk
-include src/core/directory/subdir.mk
-include src/core/context/types/subdir.mk
-include src/core/context/subdir.mk
-include src/core/subdir.mk
-include src/benchMarks/tm/list/subdir.mk
-include src/benchMarks/tm/bank/subdir.mk
-include src/benchMarks/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: ssrg-hyflow-cpp

# Tool invocations
ssrg-hyflow-cpp: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o "ssrg-hyflow-cpp" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C++_DEPS)$(C_DEPS)$(CC_DEPS)$(CPP_DEPS)$(EXECUTABLES)$(CXX_DEPS)$(C_UPPER_DEPS) ssrg-hyflow-cpp
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
