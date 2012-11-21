################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/benchMarks/tm/tpcc/TpccBenchmark.cpp \
../src/benchMarks/tm/tpcc/TpccCustomer.cpp \
../src/benchMarks/tm/tpcc/TpccDistrict.cpp \
../src/benchMarks/tm/tpcc/TpccHistory.cpp \
../src/benchMarks/tm/tpcc/TpccItem.cpp \
../src/benchMarks/tm/tpcc/TpccNewOrder.cpp \
../src/benchMarks/tm/tpcc/TpccOps.cpp \
../src/benchMarks/tm/tpcc/TpccOrder.cpp \
../src/benchMarks/tm/tpcc/TpccOrderLine.cpp \
../src/benchMarks/tm/tpcc/TpccStock.cpp \
../src/benchMarks/tm/tpcc/TpccWareHouse.cpp 

OBJS += \
./src/benchMarks/tm/tpcc/TpccBenchmark.o \
./src/benchMarks/tm/tpcc/TpccCustomer.o \
./src/benchMarks/tm/tpcc/TpccDistrict.o \
./src/benchMarks/tm/tpcc/TpccHistory.o \
./src/benchMarks/tm/tpcc/TpccItem.o \
./src/benchMarks/tm/tpcc/TpccNewOrder.o \
./src/benchMarks/tm/tpcc/TpccOps.o \
./src/benchMarks/tm/tpcc/TpccOrder.o \
./src/benchMarks/tm/tpcc/TpccOrderLine.o \
./src/benchMarks/tm/tpcc/TpccStock.o \
./src/benchMarks/tm/tpcc/TpccWareHouse.o 

CPP_DEPS += \
./src/benchMarks/tm/tpcc/TpccBenchmark.d \
./src/benchMarks/tm/tpcc/TpccCustomer.d \
./src/benchMarks/tm/tpcc/TpccDistrict.d \
./src/benchMarks/tm/tpcc/TpccHistory.d \
./src/benchMarks/tm/tpcc/TpccItem.d \
./src/benchMarks/tm/tpcc/TpccNewOrder.d \
./src/benchMarks/tm/tpcc/TpccOps.d \
./src/benchMarks/tm/tpcc/TpccOrder.d \
./src/benchMarks/tm/tpcc/TpccOrderLine.d \
./src/benchMarks/tm/tpcc/TpccStock.d \
./src/benchMarks/tm/tpcc/TpccWareHouse.d 


# Each subdirectory must supply rules for building sources it contributes
src/benchMarks/tm/tpcc/%.o: ../src/benchMarks/tm/tpcc/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DDEBUG -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


