cmake_minimum_required(VERSION 3.27)
project(Better_PSO)
link_libraries("-static")
set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} main.cpp)

# Add third party libraries.
add_subdirectory(libs)
target_include_directories(${PROJECT_NAME} PUBLIC include)

# CPack Configuration
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}")
include(CPack)