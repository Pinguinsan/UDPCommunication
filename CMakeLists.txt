#CMakeLists.txt -> UDPCommunication

cmake_minimum_required (VERSION 3.1) 
project(UDPCommunication CXX)

set(CMAKE_BUILD_TYPE Release)
set(SOURCE_BASE /opt/GitHub/UDPCommunication/)
set(CMAKE_CXX_STANDARD 14)

IF (CYGWIN OR WIN32)
    set(GCC_ADDITIONAL_COMPILE_FLAGS "")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_ADDITIONAL_COMPILE_FLAGS}")
ELSE()
    set(GCC_ADDITIONAL_COMPILE_FLAGS "-fsanitize=undefined -frecord-gcc-switches")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_ADDITIONAL_COMPILE_FLAGS}")
ENDIF()


include_directories("${SOURCE_BASE}/include")

set (UDPCOMM_SOURCES "${SOURCE_BASE}/src/udpcomm.cpp"
                     "${SOURCE_BASE}/src/udpcommunicationstrings.cpp")
 
add_executable(udpcomm ${UDPCOMM_SOURCES})
target_link_libraries(udpcomm tjlutils pthread)
