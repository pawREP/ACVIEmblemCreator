cmake_minimum_required(VERSION 3.13)  
project(EmblemCreator)               

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std:c++latest")
set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std:c++latest")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_UNICODE /DUNICODE")

file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.c 
                    ${PROJECT_SOURCE_DIR}/src/*.h 
                    ${PROJECT_SOURCE_DIR}/src/*.hpp 
                    ${PROJECT_SOURCE_DIR}/src/*.cpp
                    ${PROJECT_SOURCE_DIR}/src/*.rc
                    )

add_subdirectory(${PROJECT_SOURCE_DIR}/thirdparty/zlib)
add_executable(EmblemCreator ${SRC_FILES})

target_link_libraries(EmblemCreator PUBLIC zlib)

get_target_property(LIBA_INCLUDES zlib INCLUDE_DIRECTORIES)
target_include_directories(EmblemCreator PRIVATE ${LIBA_INCLUDES})
