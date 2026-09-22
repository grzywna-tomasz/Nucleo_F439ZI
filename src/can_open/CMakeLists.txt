cmake_minimum_required(VERSION 3.10)

project(can_open)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE
        ../../ext/CanOpenSTM32/CANopenNode/CANopen.c
        ../../ext/CanOpenSTM32/CANopenNode/CANopen.c
        ../../ext/CanOpenSTM32/CANopenNode_STM32/CO_app_STM32.c
        ../../ext/CanOpenSTM32/CANopenNode_STM32/CO_driver_STM32.c
        ../../ext/CanOpenSTM32/CANopenNode/305/CO_LSSslave.c
        ../../ext/CanOpenSTM32/CANopenNode/303/CO_LEDs.c
        ../../ext/CanOpenSTM32/CANopenNode/301/CO_Emergency.c
        ../../ext/CanOpenSTM32/CANopenNode/301/CO_NMT_Heartbeat.c
        ../../ext/CanOpenSTM32/CANopenNode/301/CO_HBconsumer.c
        ../../ext/CanOpenSTM32/CANopenNode/301/CO_SDOserver.c
        ../../ext/CanOpenSTM32/CANopenNode_STM32/OD.c
        ../../ext/CanOpenSTM32/CANopenNode/301/CO_TIME.c
        ../../ext/CanOpenSTM32/CANopenNode/301/CO_SYNC.c
        ../../ext/CanOpenSTM32/CANopenNode/301/CO_PDO.c
        ../../ext/CanOpenSTM32/CANopenNode/301/CO_ODinterface.c
        ../../ext/CanOpenSTM32/CANopenNode/301/CO_SDOclient.c
        ../../ext/CanOpenSTM32/CANopenNode/301/CO_fifo.c
        src/can_open.c
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        CO_CONFIG_SDO_CLI=1
        CO_CONFIG_FIFO=CO_CONFIG_FIFO_ENABLE
        CO_CONFIG_NMT=CO_CONFIG_NMT_MASTER)


target_include_directories(${PROJECT_NAME} 
    PUBLIC
        ../../ext/CanOpenSTM32/CANopenNode_STM32
        ../../ext/CanOpenSTM32/CANopenNode
        ../../ext/CanOpenSTM32/CANopenNode/301
        inc
)

target_link_options(${PROJECT_NAME} PRIVATE --remove)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        std_libs
        stm32cubemx
        freertos_config
        freertos_kernel
)