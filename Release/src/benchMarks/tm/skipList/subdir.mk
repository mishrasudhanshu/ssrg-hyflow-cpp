################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/benchMarks/tm/skipList/SkipListBenchmark.cpp \
../src/benchMarks/tm/skipList/SkipListNode.cpp 

OBJS += \
./src/benchMarks/tm/skipList/SkipListBenchmark.o \
./src/benchMarks/tm/skipList/SkipListNode.o 

CPP_DEPS += \
./src/benchMarks/tm/skipList/SkipListBenchmark.d \
./src/benchMarks/tm/skipList/SkipListNode.d 


# Each subdirectory must supply rules for building sources it contributes
src/benchMarks/tm/skipList/%.o: ../src/benchMarks/tm/skipList/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DRELEASE -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


