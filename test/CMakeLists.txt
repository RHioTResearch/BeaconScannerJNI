
project(ScannerTests)

add_executable(testScanner testScanner.cpp)
target_link_libraries (testScanner LINK_PUBLIC scannerJni bluetooth pthread)
