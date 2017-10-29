cmake_minimum_required(VERSION 3.8)
set(WProjectName WServiceApp)
project(${WProjectName})

set(CMAKE_CXX_STANDARD 11)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
endif()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../intermediate/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})

#Dependencies Start
set(WModules WUtilitiesModule;WUDPModule;WHTTPModule)
#Dependencies End

set(SOURCE_FILES main.cpp)

#Standart Source Files Start
file(GLOB WEngine_Files
        "WEngine/*.h"
        "WEngine/*.cpp"
        )
set(SOURCE_FILES ${SOURCE_FILES} ${WEngine_Files})
#Standart Source Files End

include_directories(WEngine)

add_executable(${WProjectName} ${SOURCE_FILES})

foreach(WModule ${WModules})
	add_subdirectory(${WModule})
	include_directories(${WModule}/Public)
	target_link_libraries(${WProjectName} ${WModule})
endforeach()