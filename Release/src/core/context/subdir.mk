################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/core/context/AbstractLock.cpp \
../src/core/context/AbstractLockTable.cpp \
../src/core/context/ContextManager.cpp \
../src/core/context/HyflowContextFactory.cpp \
../src/core/context/LockTable.cpp 

OBJS += \
./src/core/context/AbstractLock.o \
./src/core/context/AbstractLockTable.o \
./src/core/context/ContextManager.o \
./src/core/context/HyflowContextFactory.o \
./src/core/context/LockTable.o 

CPP_DEPS += \
./src/core/context/AbstractLock.d \
./src/core/context/AbstractLockTable.d \
./src/core/context/ContextManager.d \
./src/core/context/HyflowContextFactory.d \
./src/core/context/LockTable.d 


# Each subdirectory must supply rules for building sources it contributes
src/core/context/%.o: ../src/core/context/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DRELEASE -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


