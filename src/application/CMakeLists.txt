add_library(application
    AudioPlayerController.cpp
    MiniaudioMetaReader.cpp
    PlaybackScheduler.cpp
)

target_include_directories(application PUBLIC ${CMAKE_SOURCE_DIR}/include)
