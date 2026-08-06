add_subdirectory(core)
add_subdirectory(infrastructure)
add_subdirectory(application)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC
    core
    infrastructure
    application
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)