cmake_minimum_required(VERSION 3.13)  
       
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std:c++latest")
set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std:c++latest")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_UNICODE /DUNICODE")

file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c 
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h 
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp 
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.rc
                    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
                    )

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zlib)
get_target_property(LIBA_INCLUDES zlib INCLUDE_DIRECTORIES)

option(LIBEMBLEM_BUILD_STATIC_LIB "Build static library" FALSE)

if(LIBEMBLEM_BUILD_STATIC_LIB)
    add_library(libEmblem ${SRC_FILES})

    target_compile_definitions(libEmblem PRIVATE LIB_EMBLEM)
    target_include_directories(libEmblem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(libEmblem PRIVATE zlib)
    target_include_directories(libEmblem PRIVATE ${LIBA_INCLUDES})
endif()

add_executable(EmblemImporter ${SRC_FILES})

target_include_directories(EmblemImporter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(EmblemImporter PRIVATE zlib)
target_include_directories(EmblemImporter PRIVATE ${LIBA_INCLUDES})
