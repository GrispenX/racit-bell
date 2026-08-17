add_library(application
    AudioPlayerController.cpp
    MiniaudioMetaReader.cpp
    PlaybackScheduler.cpp
    GoogleAuthService.cpp
)

target_link_libraries(application PUBLIC
    httplib
    jwt-cpp
    core
)

target_include_directories(application PUBLIC ${CMAKE_SOURCE_DIR}/include)
