add_library(core
    IAudioPlayer.cpp
    IStorage.cpp
    JsonSerialization.cpp
)

target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)
