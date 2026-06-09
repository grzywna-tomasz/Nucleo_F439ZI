project(can)

add_library(${PROJECT_NAME} STATIC
    src/can.cpp)

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
    det
    stm32cubemx
)

target_compile_options(${PROJECT_NAME} PRIVATE ${OPTIMIZATION_DEBUG})