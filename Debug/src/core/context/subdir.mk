################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/core/context/ContextManager.cpp \
../src/core/context/LockTable.cpp 

OBJS += \
./src/core/context/ContextManager.o \
./src/core/context/LockTable.o 

CPP_DEPS += \
./src/core/context/ContextManager.d \
./src/core/context/LockTable.d 


# Each subdirectory must supply rules for building sources it contributes
src/core/context/%.o: ../src/core/context/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DDEBUG -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


