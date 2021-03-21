cmake_minimum_required(VERSION 3.10)
project(Billboardcloud)
set(DEBUG_MODE 1)

set(TARGET Billboardcloud)

set(HEADERS)
set(SOURCES)
file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.c*)

include_directories(${CMAKE_SOURCE_DIR}/3rd)
include_directories(${CMAKE_SOURCE_DIR}/src)

source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${HEADERS} ${SOURCES})

add_executable(${TARGET} ${HEADERS} ${SOURCES})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

set_target_properties(Billboardcloud PROPERTIES 
    CMAKE_CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    LINKER_LANGUAGE CXX
)

if(DEBUG_MODE)
    set(CMAKE_BUILD_TYPE "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    target_link_libraries(${TARGET}
        "${CMAKE_SOURCE_DIR}/3rd/assimp/lib/assimp-vc142-mtd.lib"
        "${CMAKE_SOURCE_DIR}/3rd/freetype/lib/freetype.lib"
        "${CMAKE_SOURCE_DIR}/3rd/glfw/lib/glfw3.lib"
    )
    add_custom_command(TARGET Billboardcloud POST_BUILD 
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/3rd/assimp/dll/assimp-vc142-mtd.dll"
        "${CMAKE_SOURCE_DIR}/3rd/freetype/dll/freetype.dll"
        ${CMAKE_SOURCE_DIR}/bin/$<CONFIGURATION>
    )
else()
    set(CMAKE_BUILD_TYPE "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    target_link_libraries(${TARGET}
        "${CMAKE_SOURCE_DIR}/3rd/assimp/lib/assimp-vc142-mt.lib"
        "${CMAKE_SOURCE_DIR}/3rd/freetype/lib/freetype.lib"
        "${CMAKE_SOURCE_DIR}/3rd/glfw/lib/glfw3.lib"
    )
    add_custom_command(TARGET Billboardcloud POST_BUILD 
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/3rd/assimp/dll/assimp-vc142-mt.dll"
        "${CMAKE_SOURCE_DIR}/3rd/freetype/dll/freetype.dll"
        ${CMAKE_SOURCE_DIR}/bin/$<CONFIGURATION>
    )
endif()

target_compile_options(${TARGET} PRIVATE /W4 /D_CRT_SECURE_NO_WARNINGS)
target_compile_options(${TARGET} PRIVATE /fp:fast)

set_directory_properties(PROPERTIES VS_STARTUP_PROJECT ${TARGET})
