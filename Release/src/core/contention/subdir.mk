################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/core/contention/ContentionManager.cpp 

OBJS += \
./src/core/contention/ContentionManager.o 

CPP_DEPS += \
./src/core/contention/ContentionManager.d 


# Each subdirectory must supply rules for building sources it contributes
src/core/contention/%.o: ../src/core/contention/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DRELEASE -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


