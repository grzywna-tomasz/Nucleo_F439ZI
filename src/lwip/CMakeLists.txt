project(lwip)

set(LWIP_DIR ${CMAKE_SOURCE_DIR}/ext/lwip)

set(LWIP_INCLUDE_DIRS
    ${LWIP_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

include(${LWIP_DIR}/src/Filelists.cmake)

add_library(${PROJECT_NAME} STATIC
    src/sys_arch.c
    src/lwip.c)

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        inc
        ${LWIP_INCLUDE_DIRS}
)

target_link_options(${PROJECT_NAME} PRIVATE --remove)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    stm32cubemx
    freertos_kernel
    freertos_config
    ethernetif
)

target_link_libraries(lwipcore
    PRIVATE
    freertos_kernel
    freertos_config
    lwip
)
