cmake_minimum_required(VERSION 3.10)

project(main_app)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE
        src/app.c
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        inc
)

target_link_options(${PROJECT_NAME} PRIVATE --remove)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    stm32cubemx
    freertos_kernel
    freertos_config
    lwip
    std_libs
    app_erpc_c
)