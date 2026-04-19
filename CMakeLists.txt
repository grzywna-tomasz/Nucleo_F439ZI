cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_PROJECT_NAME nucleo)
project(${CMAKE_PROJECT_NAME} C ASM CXX)

add_executable(${CMAKE_PROJECT_NAME})

# This is used to trick cmake/stm32cubemx/CMakeLists.txt into knowing about the main_app lib without modifying Cube libs
set(TOOLCHAIN_LINK_LIBRARIES main_app freertos_kernel freertos_porting)

add_subdirectory(src/app)
add_subdirectory(Nucleo_F439ZI/cmake/stm32cubemx)
add_subdirectory(src/freertos_cfg)
add_subdirectory(ext/freertos_kernel)
add_subdirectory(src/lwip)
add_subdirectory(src/std_libs)
add_subdirectory(src/lan8742_stm)
add_subdirectory(src/ethernetif)
add_subdirectory(src/app_erpc_c)
add_subdirectory(src/det)