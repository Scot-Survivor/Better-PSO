cmake_minimum_required(VERSION 3.27)
project(Better_PSO LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_SDL=2")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sUSE_SDL=2")
endif()

add_executable(${PROJECT_NAME}
        main.cpp
        searches.cpp
        include/imconfig.h
        include/searchers.h)

target_include_directories(${PROJECT_NAME} PUBLIC include)

# Add third party libraries.
add_subdirectory(libs)

if(WIN32 AND NOT EMSCRIPTEN)
    link_libraries("-static")
endif()

if(EMSCRIPTEN)
    target_link_options(${PROJECT_NAME} PRIVATE
            "-sUSE_SDL=2"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sMAX_WEBGL_VERSION=2"
            "-sMIN_WEBGL_VERSION=2")
endif()

# CPack Configuration
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}")
include(CPack)