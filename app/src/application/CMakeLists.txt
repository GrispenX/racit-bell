add_library(application
    MiniaudioMetaReader.cpp
    AudioPlayerController.cpp
    DeferredPlaybackService.cpp
    ScheduleService.cpp
    GoogleAuthService.cpp
)

target_link_libraries(application PUBLIC
    httplib
    jwt-cpp
    core
)

target_include_directories(application PUBLIC ${CMAKE_SOURCE_DIR}/include)
