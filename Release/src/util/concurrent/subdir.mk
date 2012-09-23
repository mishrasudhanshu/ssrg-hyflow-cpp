################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/concurrent/ThreadMeta.cpp 

OBJS += \
./src/util/concurrent/ThreadMeta.o 

CPP_DEPS += \
./src/util/concurrent/ThreadMeta.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/concurrent/%.o: ../src/util/concurrent/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DRELEASE -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


