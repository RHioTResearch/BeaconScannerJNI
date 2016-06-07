cmake_minimum_required(VERSION 2.8)
project(BeaconScannerJNI)

set(CMAKE_BUILD_TYPE Debug)

# For profiling
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pg -std=c++11 -fPIC")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -pg")

# Libraries
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} usr/local/lib)


# Uncomment to build the stack with print debugging enabled
# Can also use: cmake -DPRINT_DEBUG=ON -DCMAKE_BUILD_TYPE=Debug ..
#add_definitions(-DPRINT_DEBUG)

set(CMAKE_VERBOSE_MAKEFILE ON)

# The name of the scanner library
set(ScannerLibName scannerJni)

# Set the BlueZ source home
set(BluezHome $ENV{BluezHome})
set(BluezLinkType $ENV{BluezLinkType})

if("${BluezHome}" STREQUAL "")
    message("No BluezHome environment variable set, looking...")
    # Check for 5.39 version under ~/Dev
    if(EXISTS $ENV{HOME}/Dev/bluez-5.39)
        set(BluezHome $ENV{HOME}/Dev/bluez-5.39)
        message(STATUS "BluezHome found at: ${BluezHome}")
    elseif(EXISTS $ENV{HOME}/bluez-5.39)
        set(BluezHome $ENV{HOME}/bluez-5.39)
        message(STATUS "BluezHome found at: ${BluezHome}")
    else()
        # Default to a download in my envrionment
        set(BluezHome $ENV{HOME}/Dev/IoT/BLE/BlueZ/bluez-5.39)
        message(STATUS "BluezHome defaulting to ${BluezHome}")
    endif()
elseif("${BluezHome}" MATCHES "4.101$")
    message("Using legacy bluez-4.101")
    add_definitions(-DLEGACY_BLUEZ)
endif()
message("Using BluezHome=${BluezHome}")
set(BluezToolsParser ${BluezHome}/tools/parser)
include_directories(. ${BluezHome}/lib ${BluezHome}/tools /usr/include/bluetooth /usr/local/include/bluetooth /usr/include/x86_64-linux-gnu/ )
link_directories(${BLuezHome}/lib/.libs /usr/local/lib /usr/lib)
if("${BluezLinkType}" STREQUAL "static")
    #
    set(ScannerLibName ${ScannerLibName}-staticbluez)
    message("Using bluez static library, scannerLibName=${ScannerLibName}")
    ADD_LIBRARY(bluetooth STATIC IMPORTED)
    SET_TARGET_PROPERTIES(bluetooth PROPERTIES IMPORTED_LOCATION /usr/local/lib/libbluetooth.a)
    SET_TARGET_PROPERTIES(bluetooth PROPERTIES LINKER_LANGUAGE CXX)
endif()

# Set JAVA_HOME from environment, or default to known development locations
set(JAVA_HOME $ENV{JAVA_HOME})
if("${JAVA_HOME}" STREQUAL "" OR NOT EXISTS ${JAVA_HOME})
    if(NOT EXISTS ${JAVA_HOME})
        message(WARNING "JAVA_HOME=${JAVA_HOME} is invalid, searching for valid option")
    else()
        message(STATUS "JAVA_HOME not set in envrionment, attempting to default...")
    endif()
    # Check for the oracle jvm on our debian image
    if(EXISTS /usr/lib/jvm/jdk-8-oracle-arm-vfp-hflt)
        set(JAVA_HOME /usr/lib/jvm/jdk-8-oracle-arm-vfp-hflt)
    elseif(EXISTS /usr/lib/jvm/jdk1.8.0_92)
        set(JAVA_HOME /usr/lib/jvm/jdk1.8.0_92)
        message(STATUS "Using Oracle JDK=${JAVA_HOME}")
    elseif(EXISTS /Library/Java/JavaVirtualMachines/jdk1.8.0_77/Contents/Home)
        set(JAVA_HOME /Library/Java/JavaVirtualMachines/jdk1.8.0_77.jdk/Contents/Home)
        message(STATUS "Using Mac JDK=${JAVA_HOME}")
    else()
    # Default to a download in my envrionment
        set(JAVA_HOME /Library/Java/JavaVirtualMachines/jdk1.8.0_77.jdk/Contents/Home)
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
add_library (${ScannerLibName} SHARED src/org_jboss_rhiot_beacon_bluez_HCIDump.cpp src/org_jboss_rhiot_ble_bluez_HCIDump.cpp
        src/hcidumpinternal.cpp src/parser.c)
target_link_libraries(${ScannerLibName} bluetooth)
install(TARGETS ${ScannerLibName}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

add_subdirectory(test)
