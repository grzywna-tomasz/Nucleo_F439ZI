project(det)

add_library(${PROJECT_NAME} STATIC
    ../../ext/det/src/det.c)

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        ../../ext/det/inc
        inc
)

target_link_options(${PROJECT_NAME} PRIVATE --remove)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    std_libs
    app_erpc_c
)
