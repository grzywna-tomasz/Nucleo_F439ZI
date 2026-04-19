project(app_erpc_c)

add_library(${PROJECT_NAME} STATIC
    src/server.c
    src/transport_lwip_tcp.cpp
    ../erpc_generated/c_example_server.cpp
    ../erpc_generated/example_server.cpp
    ../erpc_generated/example_interface.cpp
    ../../ext/erpc/erpc_c/setup/erpc_server_setup.cpp
    ../../ext/erpc/erpc_c/setup/erpc_setup_mbf_dynamic.cpp
    ../../ext/erpc/erpc_c/infra/erpc_simple_server.cpp
    ../../ext/erpc/erpc_c/infra/erpc_basic_codec.cpp
    ../../ext/erpc/erpc_c/infra/erpc_crc16.cpp
    ../../ext/erpc/erpc_c/infra/erpc_server.cpp
    ../../ext/erpc/erpc_c/infra/erpc_message_buffer.cpp
    ../../ext/erpc/erpc_c/infra/erpc_framed_transport.cpp
    ../../ext/erpc/erpc_c/port/erpc_threading_freertos.cpp
    ../../ext/erpc/erpc_c/port/erpc_port_freertos.cpp)

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        inc
        ../erpc_generated
        ../../ext/erpc/erpc_c/setup
        ../../ext/erpc/erpc_c/port
        ../../ext/erpc/erpc_c/transports
        ../../ext/erpc/erpc_c/infra
)

target_link_options(${PROJECT_NAME} PRIVATE --remove)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    lwip
    freertos_kernel
    freertos_config
    std_libs
    main_app
    det
)