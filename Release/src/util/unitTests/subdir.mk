################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/unitTests/unitTests.cpp 

OBJS += \
./src/util/unitTests/unitTests.o 

CPP_DEPS += \
./src/util/unitTests/unitTests.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/unitTests/%.o: ../src/util/unitTests/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


