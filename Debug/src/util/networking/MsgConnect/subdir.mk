################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/networking/MsgConnect/MCtest.cpp \
../src/util/networking/MsgConnect/MSCMessage.cpp \
../src/util/networking/MsgConnect/MSCNetwork.cpp 

OBJS += \
./src/util/networking/MsgConnect/MCtest.o \
./src/util/networking/MsgConnect/MSCMessage.o \
./src/util/networking/MsgConnect/MSCNetwork.o 

CPP_DEPS += \
./src/util/networking/MsgConnect/MCtest.d \
./src/util/networking/MsgConnect/MSCMessage.d \
./src/util/networking/MsgConnect/MSCNetwork.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/networking/MsgConnect/%.o: ../src/util/networking/MsgConnect/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/mnt/data/RTSL_Backup/ubuntu_one/workspace/MsgConnect/Code/C++/Core -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


