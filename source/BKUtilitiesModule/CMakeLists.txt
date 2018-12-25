# Copyright Burak Kara, All rights reserved.

cmake_minimum_required(VERSION 3.8)
project(BKUtilitiesModule)

#Dependencies Start
#set(BKDepModsOf_BKUtilitiesModule OTHERMODULE)
#Dependencies End

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
endif()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../intermediate/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../binaries/${CMAKE_BUILD_TYPE})

include_directories(Public)
include_directories(Private)

file(GLOB Public_Files
        "Public/*.h"
        "Public/*.cpp"
        "Public/*/*.h"
        "Public/*/*.cpp"
        )
file(GLOB Private_Files
        "Private/*.h"
        "Private/*.cpp"
        "Private/*/*.h"
        "Private/*/*.cpp"
        )

set(SOURCE_FILES ${Public_Files} ${Private_Files})

add_library(BKUtilitiesModule SHARED ${SOURCE_FILES})

set_target_properties(BKUtilitiesModule PROPERTIES LINKER_LANGUAGE CXX)

foreach(BKModule ${BKDepModsOf_BKUtilitiesModule})
    include_directories(${BKModule}/Public)
    target_link_libraries(BKUtilitiesModule ${BKModule})
endforeach()