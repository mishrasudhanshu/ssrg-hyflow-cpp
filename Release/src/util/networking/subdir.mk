################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/networking/NetworkManager.cpp 

OBJS += \
./src/util/networking/NetworkManager.o 

CPP_DEPS += \
./src/util/networking/NetworkManager.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/networking/%.o: ../src/util/networking/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


