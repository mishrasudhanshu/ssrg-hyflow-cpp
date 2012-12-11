################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/benchMarks/tm/vacation/Customer.cpp \
../src/benchMarks/tm/vacation/ReservationInfo.cpp \
../src/benchMarks/tm/vacation/Resource.cpp \
../src/benchMarks/tm/vacation/Vacation.cpp \
../src/benchMarks/tm/vacation/VacationBenchmark.cpp 

OBJS += \
./src/benchMarks/tm/vacation/Customer.o \
./src/benchMarks/tm/vacation/ReservationInfo.o \
./src/benchMarks/tm/vacation/Resource.o \
./src/benchMarks/tm/vacation/Vacation.o \
./src/benchMarks/tm/vacation/VacationBenchmark.o 

CPP_DEPS += \
./src/benchMarks/tm/vacation/Customer.d \
./src/benchMarks/tm/vacation/ReservationInfo.d \
./src/benchMarks/tm/vacation/Resource.d \
./src/benchMarks/tm/vacation/Vacation.d \
./src/benchMarks/tm/vacation/VacationBenchmark.d 


# Each subdirectory must supply rules for building sources it contributes
src/benchMarks/tm/vacation/%.o: ../src/benchMarks/tm/vacation/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DRELEASE -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


