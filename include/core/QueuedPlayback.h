#ifndef INCLUDE_CORE_QUEUEDPLAYBACK_H_
#define INCLUDE_CORE_QUEUEDPLAYBACK_H_

#include <string>
#include <chrono>

struct QueuedPlayback
{
    int Id;
    std::string AudioName;
    int Priority;
    std::chrono::system_clock::time_point EnqueuedAt;
};

#endif // INCLUDE_CORE_QUEUEDPLAYBACK_H_
