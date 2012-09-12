################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/benchMarks/tm/bus_stop/busstopcorner.cpp 

OBJS += \
./src/benchMarks/tm/bus_stop/busstopcorner.o 

CPP_DEPS += \
./src/benchMarks/tm/bus_stop/busstopcorner.d 


# Each subdirectory must supply rules for building sources it contributes
src/benchMarks/tm/bus_stop/%.o: ../src/benchMarks/tm/bus_stop/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/mnt/data/RTSL_Backup/ubuntu_one/workspace/MsgConnect/Code/C++/Core -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


