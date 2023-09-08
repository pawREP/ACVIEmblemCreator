cmake_minimum_required(VERSION 3.13)  
project(EmblemCreator)               

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std:c++latest")
set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std:c++latest")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_UNICODE /DUNICODE")

set(LIBEMBLEM_BUILD_STATIC_LIB ON CACHE BOOL "")
add_subdirectory(${PROJECT_SOURCE_DIR}/libEmblem)
add_subdirectory(${PROJECT_SOURCE_DIR}/gui)
