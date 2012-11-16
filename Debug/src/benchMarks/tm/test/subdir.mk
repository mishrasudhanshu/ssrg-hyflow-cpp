################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/benchMarks/tm/test/TestSpeedBenchmark.cpp 

OBJS += \
./src/benchMarks/tm/test/TestSpeedBenchmark.o 

CPP_DEPS += \
./src/benchMarks/tm/test/TestSpeedBenchmark.d 


# Each subdirectory must supply rules for building sources it contributes
src/benchMarks/tm/test/%.o: ../src/benchMarks/tm/test/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DDEBUG -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


