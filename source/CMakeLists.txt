# Copyright Burak Kara, All rights reserved.

cmake_minimum_required(VERSION 3.8)
set(BKProjectName BKServiceApp)
project(${BKProjectName})

set(CMAKE_CXX_STANDARD 11)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wl,-allow-multiple-definition -static -static-libgcc -static-libstdc++")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
endif()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../intermediate/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})

#Dependencies Start
file(GLOB BKDependenciesFile "Dependencies.bkdep")
if(EXISTS ${BKDependenciesFile})
    file(STRINGS "Dependencies.bkdep" BKDependenciesFile_Content)
    set(BKModules ${BKDependenciesFile_Content})
else()
    file(STRINGS "BKExamples/Dependencies.bkdep" Default_BKDependenciesFile_Content)
    set(BKModules ${Default_BKDependenciesFile_Content})
endif()
#Dependencies End

#Service Main File Check Starts
file(GLOB BKServiceMainFile "main.cpp")
if(EXISTS ${BKServiceMainFile})
	set(SOURCE_FILES main.cpp)
else()
	set(SOURCE_FILES BKExamples/main.cpp)
endif()
#Service Main File Check Ends

#Standart Source Files Start
file(GLOB BKEngine_Files
        "BKEngine/*.h"
        "BKEngine/*.cpp"
        )
set(SOURCE_FILES ${SOURCE_FILES} ${BKEngine_Files})
#Standart Source Files End

include_directories(BKEngine)

add_executable(${BKProjectName} ${SOURCE_FILES})

set_target_properties(${BKProjectName} PROPERTIES LINKER_LANGUAGE CXX)

foreach(BKModule ${BKModules})
	add_subdirectory(${BKModule})
	include_directories(${BKModule}/Public)
	target_link_libraries(${BKProjectName} ${BKModule})
endforeach()