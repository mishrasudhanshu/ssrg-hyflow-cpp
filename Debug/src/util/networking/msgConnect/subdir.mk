################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/networking/msgConnect/MSCNetwork.cpp \
../src/util/networking/msgConnect/MSCtest.cpp 

OBJS += \
./src/util/networking/msgConnect/MSCNetwork.o \
./src/util/networking/msgConnect/MSCtest.o 

CPP_DEPS += \
./src/util/networking/msgConnect/MSCNetwork.d \
./src/util/networking/msgConnect/MSCtest.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/networking/msgConnect/%.o: ../src/util/networking/msgConnect/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


