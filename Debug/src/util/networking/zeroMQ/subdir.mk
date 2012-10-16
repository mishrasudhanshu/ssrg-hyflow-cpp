################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/networking/zeroMQ/ZMQNetwork.cpp \
../src/util/networking/zeroMQ/ZMQNetworkAsync.cpp 

OBJS += \
./src/util/networking/zeroMQ/ZMQNetwork.o \
./src/util/networking/zeroMQ/ZMQNetworkAsync.o 

CPP_DEPS += \
./src/util/networking/zeroMQ/ZMQNetwork.d \
./src/util/networking/zeroMQ/ZMQNetworkAsync.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/networking/zeroMQ/%.o: ../src/util/networking/zeroMQ/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DDEBUG -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


