cmake_minimum_required(VERSION 3.10)

#############################################################################
# FreeRTOS configuration
add_library(freertos_config INTERFACE)

target_include_directories(freertos_config SYSTEM
INTERFACE
    inc
)

set( FREERTOS_HEAP "4" CACHE STRING "" FORCE)
set( FREERTOS_PORT "GCC_ARM_CM4F" CACHE STRING "" FORCE)

target_compile_definitions(freertos_config
  INTERFACE
    projCOVERAGE_TEST=0
)

#############################################################################
# FreeRTOS porting
project(freertos_porting)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE
        src/freertos_porting.c
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
)