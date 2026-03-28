cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_PROJECT_NAME nucleo)
project(${CMAKE_PROJECT_NAME} C ASM CXX)

add_executable(${CMAKE_PROJECT_NAME})

# This is used to trick cmake/stm32cubemx/CMakeLists.txt into knowing about the main_app lib without modifying Cube libs
set(TOOLCHAIN_LINK_LIBRARIES main_app)

add_subdirectory(src/app)
add_subdirectory(Nucleo_F439ZI/cmake/stm32cubemx)

