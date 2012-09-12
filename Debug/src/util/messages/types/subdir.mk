################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/messages/types/LockAccessMsg.cpp \
../src/util/messages/types/ObjectAccessMsg.cpp \
../src/util/messages/types/ObjectTrackerMsg.cpp \
../src/util/messages/types/ReadValidationMsg.cpp \
../src/util/messages/types/RegisterObjectMsg.cpp \
../src/util/messages/types/SynchronizeMsg.cpp 

OBJS += \
./src/util/messages/types/LockAccessMsg.o \
./src/util/messages/types/ObjectAccessMsg.o \
./src/util/messages/types/ObjectTrackerMsg.o \
./src/util/messages/types/ReadValidationMsg.o \
./src/util/messages/types/RegisterObjectMsg.o \
./src/util/messages/types/SynchronizeMsg.o 

CPP_DEPS += \
./src/util/messages/types/LockAccessMsg.d \
./src/util/messages/types/ObjectAccessMsg.d \
./src/util/messages/types/ObjectTrackerMsg.d \
./src/util/messages/types/ReadValidationMsg.d \
./src/util/messages/types/RegisterObjectMsg.d \
./src/util/messages/types/SynchronizeMsg.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/messages/types/%.o: ../src/util/messages/types/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/mnt/data/RTSL_Backup/ubuntu_one/workspace/MsgConnect/Code/C++/Core -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


