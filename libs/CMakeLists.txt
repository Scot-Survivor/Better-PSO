if(NOT EMSCRIPTEN)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(SDL_LIBC ON CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(SDL)
    find_package(OpenGL REQUIRED)
endif()

# Implot
file(GLOB IMPLOT_SRC_FILES ${CMAKE_SOURCE_DIR}/libs/implot/*.cpp ${CMAKE_SOURCE_DIR}/libs/implot/*.h)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/libs/implot)

# ImGui stuff
file(GLOB IMGUI_STANDARD_SRC_FILES ${CMAKE_SOURCE_DIR}/libs/imgui/*.cpp ${CMAKE_SOURCE_DIR}/libs/imgui/*.h)
list(APPEND IMGUI_STANDARD_SRC_FILES ${IMPLOT_SRC_FILES})
list(REMOVE_ITEM IMGUI_STANDARD_SRC_FILES ${CMAKE_SOURCE_DIR}/libs/imgui/imconfig.h)

add_library(imgui STATIC
        ${IMGUI_STANDARD_SRC_FILES}
        ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_sdl2.cpp
        ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
        ${CMAKE_SOURCE_DIR}/libs/imgui
        ${CMAKE_SOURCE_DIR}/libs/imgui/backends
        ${CMAKE_SOURCE_DIR}/libs/implot
        ${CMAKE_SOURCE_DIR}/include
)

if(NOT EMSCRIPTEN)
    target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/libs/SDL/include)
    target_link_libraries(imgui PUBLIC OpenGL::GL)
    target_link_libraries(${PROJECT_NAME} SDL2::SDL2main SDL2::SDL2-static)
endif()

target_link_libraries(${PROJECT_NAME} imgui)
