#ifndef INCLUDE_CORE_PLAYBACK_H_
#define INCLUDE_CORE_PLAYBACK_H_

#include <string>
#include <chrono>

struct Playback
{
    int Id = 0;
    std::string AudioName;
    int Priority;
    std::chrono::system_clock::time_point Time;
};

#endif // INCLUDE_CORE_PLAYBACK_H_
