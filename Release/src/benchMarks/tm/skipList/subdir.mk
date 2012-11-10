################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/benchMarks/tm/skipList/skipListNode.cpp 

OBJS += \
./src/benchMarks/tm/skipList/skipListNode.o 

CPP_DEPS += \
./src/benchMarks/tm/skipList/skipListNode.d 


# Each subdirectory must supply rules for building sources it contributes
src/benchMarks/tm/skipList/%.o: ../src/benchMarks/tm/skipList/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DRELEASE -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


