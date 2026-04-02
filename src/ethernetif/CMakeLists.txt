project(ethernetif)

add_library(${PROJECT_NAME} STATIC
    src/ethernetif.c)

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        inc
)

target_link_options(${PROJECT_NAME} PRIVATE --remove)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    stm32cubemx
    lwip
    lwipcore
    freertos_config
    freertos_kernel
    lan8742_stm
)