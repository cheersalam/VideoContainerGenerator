# Mohammad Alam
# cheersalam@gmail.com
# cmake file

cmake_minimum_required (VERSION 3.0)
set(PROJECT_NAME "BebopVideoStreamer")
project (${PROJECT_NAME})

#------------------------------------------------------------------------------
# Compiler Flags options 
#------------------------------------------------------------------------------
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin" CACHE PATH "Output directory for applications" )
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    MESSAGE(" Configuring flags for GNU:")
    set(VS_C_FLAGS "-Wall -Wextra -W ")
    set(VS_C_FLAGS_RELEASE "-O2")
    set(VS_C_FLAGS_DEBUG "-g3 -O0")
    set(VS_CXX_FLAGS "-Wall -Wextra -W")
    set(VS_CXX_FLAGS_RELEASE "-O2")
    set(VS_CXX_FLAGS_DEBUG "-g3 -O0")
endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

#------------------------------------------------------------------------------
#Settings MSVC Macros
#------------------------------------------------------------------------------
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    MESSAGE(" Configuring flags for MSVC:")
    set(VS_C_FLAGS "/W4 /WX /EHsc")
    set(VS_C_FLAGS_RELEASE "-O2")
    set(VS_C_FLAGS_DEBUG "-g3 -O0")
    set(VS_CXX_FLAGS "/W4 /WX /EHsc")
    set(VS_CXX_FLAGS_RELEASE "-O2")
    set(VS_CXX_FLAGS_DEBUG "-g3 -O0")
endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

#only append flags to existing flags
set(CMAKE_C_FLAGS           "${CMAKE_C_FLAGS} ${VS_C_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} ${VS_C_FLAGS_RELEASE}")
set(CMAKE_C_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG} ${VS_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} ${VS_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${VS_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} ${VS_CXX_FLAGS_DEBUG}")

set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}
    CACHE STRING "Choose the type of build: None Debug Release"
    FORCE)

#set(LIB_INSTALL_DIR lib)
include_directories(include/ )
link_directories(${PROJECT_BINARY_DIR}/lib/)

#------------------------------------------------------------------------------
# Project Settings: path for lib, bin and include folder
#------------------------------------------------------------------------------
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin" CACHE PATH "Output directory for applications" )
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib" CACHE PATH "Output Directory for Libs")
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/include")
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

#------------------------------------------------------------------------------
# Project Settings: Debug or Release 
#------------------------------------------------------------------------------
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin" CACHE PATH "Output directory for applications" )
get_filename_component(COMPILE_TYPE ${CMAKE_CURRENT_BINARY_DIR} NAME)
if( ${COMPILE_TYPE} STREQUAL "debug" OR 
    ${COMPILE_TYPE} STREQUAL "Debug")
        set(CMAKE_BUILD_TYPE "Debug")
else()
    set(CMAKE_BUILD_TYPE "Release")
endif()

find_Package(SDL REQUIRED)

#------------------------------------------------------------------------------
# External Project - ffmpeg
#------------------------------------------------------------------------------
include(ExternalProject)
ExternalProject_Add(
                    ffmpeg
                    GIT_REPOSITORY https://github.com/FFmpeg/FFmpeg.git
                    GIT_TAG master
                    UPDATE_COMMAND ""
                    SOURCE_DIR ${PROJECT_SOURCE_DIR}/thirdparty/ffmpeg
                    BUILD_IN_SOURCE 1
                    BUILD_ALWAYS 0
                    CONFIGURE_COMMAND ./configure  --enable-pthreads --enable-static --enable-shared --prefix=${PROJECT_BINARY_DIR}
                    BUILD_COMMAND make
                    INSTALL_COMMAND make install)

#------------------------------------------------------------------------------
# Adding FFMPEG lib
#------------------------------------------------------------------------------
set(FFMPEG_LIBS avcodec avdevice avfilter avformat avutil swresample swscale)

#------------------------------------------------------------------------------
# Adding containerGenerator Lib
#------------------------------------------------------------------------------
set(FFMPEG_CONTAINER_GEN ffmpegWrapper)

#------------------------------------------------------------------------------
# Add all servers lib dependecies here. (/trunk/src/servers) 
#------------------------------------------------------------------------------
set(PROJECT_LIBS ${FFMPEG_CONTAINER_GEN} ${FFMPEG_LIBS} -lSDL -lm)

#------------------------------------------------------------------------------
# Adding colors 
#------------------------------------------------------------------------------
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)

#------------------------------------------------------------------------------
# Sub Directories 
#------------------------------------------------------------------------------
add_subdirectory (ffmpegWrapper) 
add_subdirectory (mpegtsTest) 

#------------------------------------------------------------------------------
# Configuration Summary 
#------------------------------------------------------------------------------
MESSAGE("")
MESSAGE(" Platform:")
MESSAGE("   Host:             "  ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_VERSION} ${CMAKE_HOST_SYSTEM_PROCESSOR})
MESSAGE("   CMake Version:    "  ${CMAKE_VERSION})
if(MSVC)
    MESSAGE("   MSVC:             "         ${MSVC_VERSION})   
endif()

MESSAGE("   C flags           "  ${CMAKE_C_FLAGS})
MESSAGE("   C flags (Release) "  ${CMAKE_C_FLAGS_RELEASE})
MESSAGE("   C flags (Debug)   "  ${CMAKE_C_FLAGS_DEBUG})
MESSAGE("   Build Type        "  ${CMAKE_BUILD_TYPE})



