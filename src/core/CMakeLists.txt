add_library(core
    JsonSerialization.cpp
)

target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)
