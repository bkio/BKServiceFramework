cmake_minimum_required(VERSION 3.8)
set(WProjectName WServiceApp)
project(${WProjectName})

set(CMAKE_CXX_STANDARD 11)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
endif()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../intermediate/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})

#Dependencies Start
file(GLOB WDependenciesFile "WDependencies.wdep")
if(EXISTS ${WDependenciesFile})
    file(STRINGS "WDependencies.wdep" WDependenciesFile_Content)
    set(WModules ${WDependenciesFile_Content})
else()
    file(STRINGS "WExamples/WDependencies.wdep" Default_WDependenciesFile_Content)
    set(WModules ${Default_WDependenciesFile_Content})
endif()
#Dependencies End

#Service Main File Check Starts
file(GLOB WServiceMainFile "main.cpp")
if(EXISTS ${WServiceMainFile})
	set(SOURCE_FILES main.cpp)
else()
	set(SOURCE_FILES WExamples/main.cpp)
endif()
#Service Main File Check Ends

#Standart Source Files Start
file(GLOB WEngine_Files
        "WEngine/*.h"
        "WEngine/*.cpp"
        )
set(SOURCE_FILES ${SOURCE_FILES} ${WEngine_Files})
#Standart Source Files End

include_directories(WEngine)

add_executable(${WProjectName} ${SOURCE_FILES})

set_target_properties(${WProjectName} PROPERTIES LINKER_LANGUAGE CXX)

foreach(WModule ${WModules})
	add_subdirectory(${WModule})
	include_directories(${WModule}/Public)
	target_link_libraries(${WProjectName} ${WModule})
endforeach()