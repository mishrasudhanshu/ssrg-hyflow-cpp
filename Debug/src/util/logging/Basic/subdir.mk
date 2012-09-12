################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/logging/Basic/BasicLogLib.cpp 

OBJS += \
./src/util/logging/Basic/BasicLogLib.o 

CPP_DEPS += \
./src/util/logging/Basic/BasicLogLib.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/logging/Basic/%.o: ../src/util/logging/Basic/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/mnt/data/RTSL_Backup/ubuntu_one/workspace/MsgConnect/Code/C++/Core -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


