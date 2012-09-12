################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/networking/msgConnect/MsgConnect/MCBase.cpp \
../src/util/networking/msgConnect/MsgConnect/MCDataTree.cpp \
../src/util/networking/msgConnect/MsgConnect/MCFile.cpp \
../src/util/networking/msgConnect/MsgConnect/MCHTTP.cpp \
../src/util/networking/msgConnect/MsgConnect/MCInetTransport.cpp \
../src/util/networking/msgConnect/MsgConnect/MCLists.cpp \
../src/util/networking/msgConnect/MsgConnect/MCLogger.cpp \
../src/util/networking/msgConnect/MsgConnect/MCMMF.cpp \
../src/util/networking/msgConnect/MsgConnect/MCPlatforms.cpp \
../src/util/networking/msgConnect/MsgConnect/MCSock.cpp \
../src/util/networking/msgConnect/MsgConnect/MCSocket.cpp \
../src/util/networking/msgConnect/MsgConnect/MCStream.cpp \
../src/util/networking/msgConnect/MsgConnect/MCStreams.cpp \
../src/util/networking/msgConnect/MsgConnect/MCSyncs.cpp \
../src/util/networking/msgConnect/MsgConnect/MCThreads.cpp \
../src/util/networking/msgConnect/MsgConnect/MCUDPSocket.cpp \
../src/util/networking/msgConnect/MsgConnect/MCUtils.cpp 

OBJS += \
./src/util/networking/msgConnect/MsgConnect/MCBase.o \
./src/util/networking/msgConnect/MsgConnect/MCDataTree.o \
./src/util/networking/msgConnect/MsgConnect/MCFile.o \
./src/util/networking/msgConnect/MsgConnect/MCHTTP.o \
./src/util/networking/msgConnect/MsgConnect/MCInetTransport.o \
./src/util/networking/msgConnect/MsgConnect/MCLists.o \
./src/util/networking/msgConnect/MsgConnect/MCLogger.o \
./src/util/networking/msgConnect/MsgConnect/MCMMF.o \
./src/util/networking/msgConnect/MsgConnect/MCPlatforms.o \
./src/util/networking/msgConnect/MsgConnect/MCSock.o \
./src/util/networking/msgConnect/MsgConnect/MCSocket.o \
./src/util/networking/msgConnect/MsgConnect/MCStream.o \
./src/util/networking/msgConnect/MsgConnect/MCStreams.o \
./src/util/networking/msgConnect/MsgConnect/MCSyncs.o \
./src/util/networking/msgConnect/MsgConnect/MCThreads.o \
./src/util/networking/msgConnect/MsgConnect/MCUDPSocket.o \
./src/util/networking/msgConnect/MsgConnect/MCUtils.o 

CPP_DEPS += \
./src/util/networking/msgConnect/MsgConnect/MCBase.d \
./src/util/networking/msgConnect/MsgConnect/MCDataTree.d \
./src/util/networking/msgConnect/MsgConnect/MCFile.d \
./src/util/networking/msgConnect/MsgConnect/MCHTTP.d \
./src/util/networking/msgConnect/MsgConnect/MCInetTransport.d \
./src/util/networking/msgConnect/MsgConnect/MCLists.d \
./src/util/networking/msgConnect/MsgConnect/MCLogger.d \
./src/util/networking/msgConnect/MsgConnect/MCMMF.d \
./src/util/networking/msgConnect/MsgConnect/MCPlatforms.d \
./src/util/networking/msgConnect/MsgConnect/MCSock.d \
./src/util/networking/msgConnect/MsgConnect/MCSocket.d \
./src/util/networking/msgConnect/MsgConnect/MCStream.d \
./src/util/networking/msgConnect/MsgConnect/MCStreams.d \
./src/util/networking/msgConnect/MsgConnect/MCSyncs.d \
./src/util/networking/msgConnect/MsgConnect/MCThreads.d \
./src/util/networking/msgConnect/MsgConnect/MCUDPSocket.d \
./src/util/networking/msgConnect/MsgConnect/MCUtils.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/networking/msgConnect/MsgConnect/%.o: ../src/util/networking/msgConnect/MsgConnect/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


