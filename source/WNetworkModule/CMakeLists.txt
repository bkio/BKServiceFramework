cmake_minimum_required(VERSION 3.8)
project(WNetworkModule)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../binaries/${CMAKE_BUILD_TYPE})

include_directories(Public)
include_directories(Private)
include_directories(WEngine)

file(GLOB Public_Files
        "Public/*.h"
        "Public/*.cpp"
        )
file(GLOB Private_Files
        "Private/*.h"
        "Private/*.cpp"
        )

set(SOURCE_FILES ${Public_Files} ${Private_Files})

add_library(WNetworkModule SHARED ${SOURCE_FILES})