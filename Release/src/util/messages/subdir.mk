################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/messages/HyflowMessage.cpp \
../src/util/messages/HyflowMessageFuture.cpp \
../src/util/messages/MessageHandler.cpp \
../src/util/messages/MessageMaps.cpp 

OBJS += \
./src/util/messages/HyflowMessage.o \
./src/util/messages/HyflowMessageFuture.o \
./src/util/messages/MessageHandler.o \
./src/util/messages/MessageMaps.o 

CPP_DEPS += \
./src/util/messages/HyflowMessage.d \
./src/util/messages/HyflowMessageFuture.d \
./src/util/messages/MessageHandler.d \
./src/util/messages/MessageMaps.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/messages/%.o: ../src/util/messages/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DRELEASE -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


