################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/benchMarks/tm/bank/BankAccount.cpp \
../src/benchMarks/tm/bank/BankBenchmark.cpp 

OBJS += \
./src/benchMarks/tm/bank/BankAccount.o \
./src/benchMarks/tm/bank/BankBenchmark.o 

CPP_DEPS += \
./src/benchMarks/tm/bank/BankAccount.d \
./src/benchMarks/tm/bank/BankBenchmark.d 


# Each subdirectory must supply rules for building sources it contributes
src/benchMarks/tm/bank/%.o: ../src/benchMarks/tm/bank/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DRELEASE -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


