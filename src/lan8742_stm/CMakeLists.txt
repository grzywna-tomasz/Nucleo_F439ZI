project(lan8742_stm)

add_library(${PROJECT_NAME} STATIC
    src/lan8742.c)

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        inc
)

target_link_options(${PROJECT_NAME} PRIVATE --remove)
