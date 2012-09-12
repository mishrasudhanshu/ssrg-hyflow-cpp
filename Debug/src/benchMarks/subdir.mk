################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/benchMarks/Application.cpp \
../src/benchMarks/BenchmarkExecutor.cpp 

OBJS += \
./src/benchMarks/Application.o \
./src/benchMarks/BenchmarkExecutor.o 

CPP_DEPS += \
./src/benchMarks/Application.d \
./src/benchMarks/BenchmarkExecutor.d 


# Each subdirectory must supply rules for building sources it contributes
src/benchMarks/%.o: ../src/benchMarks/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


