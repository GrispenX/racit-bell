#ifndef INCLUDE_CORE_DEFERREDPLAYBACK_H_
#define INCLUDE_CORE_DEFERREDPLAYBACK_H_

#include <string>
#include <chrono>

struct DeferredPlayback
{
    int Id;
    std::string AudioName;
    int Priority;
    std::chrono::system_clock::time_point PlayTime;
};

#endif // INCLUDE_CORE_DEFERREDPLAYBACK_H_
