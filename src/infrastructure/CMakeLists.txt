add_library(infrastructure
    FsAudioStorage.cpp
    MiniaudioPlayer.cpp
)

target_link_libraries(infrastructure PUBLIC
    core
)

target_include_directories(infrastructure PUBLIC ${CMAKE_SOURCE_DIR}/include)
