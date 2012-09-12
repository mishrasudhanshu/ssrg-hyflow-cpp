################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/core/context/types/DTL2Context.cpp 

OBJS += \
./src/core/context/types/DTL2Context.o 

CPP_DEPS += \
./src/core/context/types/DTL2Context.d 


# Each subdirectory must supply rules for building sources it contributes
src/core/context/types/%.o: ../src/core/context/types/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


