
project(ScannerTests)

add_executable(testScanner testScanner.cpp)
target_link_libraries (testScanner LINK_PUBLIC ${ScannerLibName} bluetooth pthread)

add_executable(testGeneralScanner testGeneralScanner.cpp)
target_link_libraries (testGeneralScanner LINK_PUBLIC ${ScannerLibName} bluetooth pthread)

add_executable(testRHIoTTag testRHIoTTag.cpp)
target_link_libraries (testRHIoTTag LINK_PUBLIC ${ScannerLibName} bluetooth pthread)

add_executable(testBeaconBuffer testBeaconBuffer.cpp)

add_executable(testRHIoTTagBuffer testRHIoTTagBuffer.cpp)

add_executable(testAdEventInline testAdEventInline.cpp)
