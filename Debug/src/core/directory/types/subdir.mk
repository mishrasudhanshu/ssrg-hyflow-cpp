################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/core/directory/types/TrackerDirectory.cpp 

OBJS += \
./src/core/directory/types/TrackerDirectory.o 

CPP_DEPS += \
./src/core/directory/types/TrackerDirectory.d 


# Each subdirectory must supply rules for building sources it contributes
src/core/directory/types/%.o: ../src/core/directory/types/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


