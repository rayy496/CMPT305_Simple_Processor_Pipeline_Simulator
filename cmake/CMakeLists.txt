cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

project(
	CMPT305_Project
	VERSION 1.0
	LANGUAGES CXX)

file(GLOB SOURCES "../src/*.cpp")

add_executable(proj ${SOURCES})

target_include_directories(proj PRIVATE ../include)

target_compile_options(proj PRIVATE -Wall -Werror)

# output executable object to root dir
set_target_properties(proj PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/..)