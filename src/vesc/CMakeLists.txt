cmake_minimum_required(VERSION 3.10)

project(vesc)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE
        src/vesc.cpp
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        inc
)

target_link_options(${PROJECT_NAME} PRIVATE --remove)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    freertos_kernel
    freertos_config
    std_libs
    app_erpc_c
    det
    can
    stm32cubemx
)

target_compile_options(${PROJECT_NAME} PRIVATE ${OPTIMIZATION_DEBUG})