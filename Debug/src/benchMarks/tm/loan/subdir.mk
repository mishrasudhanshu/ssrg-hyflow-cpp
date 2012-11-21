################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/benchMarks/tm/loan/LoanAccount.cpp \
../src/benchMarks/tm/loan/LoanBenchMark.cpp 

OBJS += \
./src/benchMarks/tm/loan/LoanAccount.o \
./src/benchMarks/tm/loan/LoanBenchMark.o 

CPP_DEPS += \
./src/benchMarks/tm/loan/LoanAccount.d \
./src/benchMarks/tm/loan/LoanBenchMark.d 


# Each subdirectory must supply rules for building sources it contributes
src/benchMarks/tm/loan/%.o: ../src/benchMarks/tm/loan/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DDEBUG -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


