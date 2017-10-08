cmake_minimum_required(VERSION 3.8)
set(WProjectName WUDPGateway)
project(${WProjectName})

set(CMAKE_CXX_STANDARD 11)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE}/archives)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../binaries/${CMAKE_BUILD_TYPE})

set(Boost_USE_STATIC_LIBS        ON) # only find static libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../libraries/boost_1_65_1")

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

find_package(Boost 1.65.1)

if(Boost_FOUND)
	message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
	message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
	message(STATUS "Boost_VERSION: ${Boost_VERSION}")

    set(SOURCE_FILES ${SOURCE_FILES} main.cpp) #main.cpp file

    include_directories(WEngine)

    add_executable(${WProjectName} ${SOURCE_FILES})

	foreach(WModule ${WModules})
		add_subdirectory(${WModule})
        include_directories(${WModule}/Public)
		target_link_libraries(${WProjectName} ${WModule})
	endforeach()

	include_directories(${Boost_INCLUDE_DIRS})
	target_link_libraries(${WProjectName} ${Boost_LIBRARIES})
else()
    message(FATAL_ERROR "Could not find boost!")
endif()