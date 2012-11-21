################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/benchMarks/tm/Vacation/Customer.cpp \
../src/benchMarks/tm/Vacation/Reservation.cpp \
../src/benchMarks/tm/Vacation/ReservationInfo.cpp \
../src/benchMarks/tm/Vacation/VacationBenchmark.cpp 

OBJS += \
./src/benchMarks/tm/Vacation/Customer.o \
./src/benchMarks/tm/Vacation/Reservation.o \
./src/benchMarks/tm/Vacation/ReservationInfo.o \
./src/benchMarks/tm/Vacation/VacationBenchmark.o 

CPP_DEPS += \
./src/benchMarks/tm/Vacation/Customer.d \
./src/benchMarks/tm/Vacation/Reservation.d \
./src/benchMarks/tm/Vacation/ReservationInfo.d \
./src/benchMarks/tm/Vacation/VacationBenchmark.d 


# Each subdirectory must supply rules for building sources it contributes
src/benchMarks/tm/Vacation/%.o: ../src/benchMarks/tm/Vacation/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DDEBUG -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


