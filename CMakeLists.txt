cmake_minimum_required(VERSION 2.8)
project(BeaconScannerJNI)

# For profiling
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -std=c++11 -fPIC")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")

# Libraries
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} usr/local/lib)


# Uncomment to build the stack with print debugging enabled
# Can also use: cmake -DPRINT_DEBUG=ON -DCMAKE_BUILD_TYPE=Debug ..
#add_definitions(-DPRINT_DEBUG)

set(CMAKE_VERBOSE_MAKEFILE ON)
# BluezHome is set in the parent CMakeLists.txt file
set(BluezToolsParser ${BluezHome}/tools/parser)
include_directories(. ${BluezHome} ${BluezHome}/tools /usr/include/bluetooth  /usr/include/x86_64-linux-gnu/)

# Set JAVA_HOME from environment, or default to known development locations
set(JAVA_HOME $ENV{JAVA_HOME})
if("${JAVA_HOME}" STREQUAL "")
    message(STATUS "JAVA_HOME not set in envrionment, attempting to default...")
    # Check for the oracle jvm on our debian image
    if(EXISTS /usr/lib/jvm/jdk-8-oracle-arm-vfp-hflt)
        set(JAVA_HOME /usr/lib/jvm/jdk-8-oracle-arm-vfp-hflt)
    elseif(EXISTS /usr/lib/jvm/jdk1.8.0_51)
        set(JAVA_HOME /usr/lib/jvm/jdk1.8.0_51)
        message(STATUS "Using Oracle JDK=${JAVA_HOME}")
    elseif(EXISTS /Library/Java/JavaVirtualMachines/jdk1.8.0_45.jdk/Contents/Home)
        set(JAVA_HOME /Library/Java/JavaVirtualMachines/jdk1.8.0_45.jdk/Contents/Home)
        message(STATUS "Using Mac JDK=${JAVA_HOME}")
    else()
    # Default to a download in my envrionment
        set(JAVA_HOME /root/Downloads/jdk1.8.0_45)
        message(STATUS "JAVA_HOME defaulting to ${JAVA_HOME}")
    endif()
endif()
# Validate it has the jni headers
if(EXISTS ${JAVA_HOME}/include AND EXISTS ${JAVA_HOME}/include/linux)
    include_directories(${JAVA_HOME}/include ${JAVA_HOME}/include/linux)
    message(STATUS "Using JDK=${JAVA_HOME}")
elseif(EXISTS ${JAVA_HOME}/include AND EXISTS ${JAVA_HOME}/include/darwin)
    include_directories(${JAVA_HOME}/include ${JAVA_HOME}/include/darwin)
    message(STATUS "Using Mac JDK=${JAVA_HOME}, development only mode")
else()
    message(FATAL_ERROR "${JAVA_HOME}/include and ${JAVA_HOME}/include/linux do not exist. Check that JAVA_HOME points to a JDK.")
endif()

# The scannerJni shared library
add_library (scannerJni SHARED src/org_jboss_rhiot_beacon_bluez_HCIDump.cpp src/hcidumpinternal.c src/parser.c)
target_link_libraries(scannerJni)
install(TARGETS scannerJni
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

add_subdirectory(test)
