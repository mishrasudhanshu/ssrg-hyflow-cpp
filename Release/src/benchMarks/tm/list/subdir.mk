################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/benchMarks/tm/list/ListBenchmark.cpp \
../src/benchMarks/tm/list/ListNode.cpp 

OBJS += \
./src/benchMarks/tm/list/ListBenchmark.o \
./src/benchMarks/tm/list/ListNode.o 

CPP_DEPS += \
./src/benchMarks/tm/list/ListBenchmark.d \
./src/benchMarks/tm/list/ListNode.d 


# Each subdirectory must supply rules for building sources it contributes
src/benchMarks/tm/list/%.o: ../src/benchMarks/tm/list/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DRELEASE -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


