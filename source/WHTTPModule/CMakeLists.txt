cmake_minimum_required(VERSION 3.8)
project(WHTTPModule)

#Dependencies Start
set(WDepModsOf_WHTTPModule WUtilitiesModule)
#Dependencies End

if(WIN32)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
endif()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../binaries/${CMAKE_BUILD_TYPE})

include_directories(Public)
include_directories(Private)

file(GLOB Public_Files
        "Public/*.h"
        "Public/*.cpp"
        )
file(GLOB Private_Files
        "Private/*.h"
        "Private/*.cpp"
        )

set(SOURCE_FILES ${Public_Files} ${Private_Files})

add_library(WHTTPModule SHARED ${SOURCE_FILES})

if(WIN32)
    target_link_libraries(WHTTPModule ws2_32.lib)
    MESSAGE(STATUS "Platform: Windows")
else()
    MESSAGE(STATUS "Platform: Non-Windows")
endif()

foreach(WModule ${WDepModsOf_WHTTPModule})
    include_directories(${WModule}/Public)
    target_link_libraries(WHTTPModule ${WModule})
endforeach()