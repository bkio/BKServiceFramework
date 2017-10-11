cmake_minimum_required(VERSION 3.8)
set(WProjectName WUDPGateway)
project(${WProjectName})

set(CMAKE_CXX_STANDARD 11)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})

#Dependencies Start
set(WModules WUtilitiesModule;WNetworkModule)
#Dependencies End

#Standart Source Files Start
file(GLOB WEngine_Files
        "WEngine/*.h"
        "WEngine/*.cpp"
        )
set(SOURCE_FILES ${WEngine_Files})
#Standart Source Files End

set(SOURCE_FILES ${SOURCE_FILES} main.cpp) #main.cpp file

include_directories(WEngine)

add_executable(${WProjectName} ${SOURCE_FILES})

foreach(WModule ${WModules})
	add_subdirectory(${WModule})
	include_directories(${WModule}/Public)
	target_link_libraries(${WProjectName} ${WModule})
endforeach()