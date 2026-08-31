#ifndef INCLUDE_CORE_AUDIO_H_
#define INCLUDE_CORE_AUDIO_H_

#include <string>
#include <chrono>
#include <vector>

struct AudioMeta
{
    std::string Name;
    std::chrono::milliseconds Duration;
};

struct Audio
{
    AudioMeta Meta;
    std::vector<std::byte> Data;
};

#endif // INCLUDE_CORE_AUDIO_H_
