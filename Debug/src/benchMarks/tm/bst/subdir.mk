################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/benchMarks/tm/bst/BstBenchmark.cpp \
../src/benchMarks/tm/bst/BstNode.cpp 

OBJS += \
./src/benchMarks/tm/bst/BstBenchmark.o \
./src/benchMarks/tm/bst/BstNode.o 

CPP_DEPS += \
./src/benchMarks/tm/bst/BstBenchmark.d \
./src/benchMarks/tm/bst/BstNode.d 


# Each subdirectory must supply rules for building sources it contributes
src/benchMarks/tm/bst/%.o: ../src/benchMarks/tm/bst/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DDEBUG -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


