################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/core/exceptions/types/TransactionException.cpp 

OBJS += \
./src/core/exceptions/types/TransactionException.o 

CPP_DEPS += \
./src/core/exceptions/types/TransactionException.d 


# Each subdirectory must supply rules for building sources it contributes
src/core/exceptions/types/%.o: ../src/core/exceptions/types/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/mnt/data/RTSL_Backup/ubuntu_one/workspace/MsgConnect/Code/C++/Core -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


