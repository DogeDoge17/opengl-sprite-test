cmake_minimum_required(VERSION 3.31.6)
project(opengl_test LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
#add_compile_options(-pedantic -Wall -Wextra)

set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)


include(FetchContent)

FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.14
)
set(SDL_VIDEO_WAYLAND ON CACHE BOOL "" FORCE)
set(SDL_VIDEO_X11 OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(SDL3)
find_package(OpenGL REQUIRED)

add_library(glad glad/src/glad.c)
target_include_directories(glad PUBLIC glad/include)

add_executable(opengl_test main.c
        image_paths.h)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/norm_frag.glsl ${CMAKE_CURRENT_BINARY_DIR}/norm_frag.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/norm_vert.glsl ${CMAKE_CURRENT_BINARY_DIR}/norm_vert.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bicubic_frag.glsl ${CMAKE_CURRENT_BINARY_DIR}/bicubic_frag.glsl COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lanczos_frag.glsl ${CMAKE_CURRENT_BINARY_DIR}/lanczos_frag.glsl COPYONLY)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mod_assets)
set(DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/)
file(COPY ${SOURCE_DIR} DESTINATION ${DEST_DIR})



# Link SDL3 to your project
target_link_libraries(opengl_test PRIVATE SDL3::SDL3-static  OpenGL::GL glad)

# Suppress warnings from SDL3's headers
if (MSVC)
    target_compile_options(opengl_test PRIVATE /wd4996) # Add MSVC-specific warning suppressions here if needed
else()
    target_compile_options(opengl_test PRIVATE -isystem ${SDL3_SOURCE_DIR}/include)
endif()