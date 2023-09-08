cmake_minimum_required(VERSION 3.13)  
project(EmblemCreator)               

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std:c++latest")
set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std:c++latest")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_UNICODE /DUNICODE")

file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c 
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h 
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp 
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.rc
                    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/*.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_dx11.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_win32.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/misc/imgui_stdlib.cpp
                    )

set(BUILD_TESTING OFF CACHE BOOL "")
set(BUILD_TOOLS OFF CACHE BOOL "")
set(BUILD_XAUDIO_WIN10 ON CACHE BOOL "")
set(BUILD_XAUDIO_WIN8 OFF CACHE BOOL "")
add_subdirectory(thirdparty/DirectXTK)

add_executable(EmblemCreator ${SRC_FILES})

target_link_options(EmblemCreator PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")

set(LIBEMBLEM_BUILD_STATIC_LIB ON CACHE BOOL "")
target_link_libraries(EmblemCreator PUBLIC DirectXTK)
target_link_libraries(EmblemCreator PUBLIC libEmblem)

target_include_directories(EmblemCreator PRIVATE thirdparty/imgui)
