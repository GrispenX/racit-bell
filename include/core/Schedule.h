#ifndef INCLUDE_CORE_SCHEDULE_H_
#define INCLUDE_CORE_SCHEDULE_H_

#include <string>
#include <chrono>
#include <vector>

struct ScheduleEntry
{
    std::string AudioName;
    int Priority;
    std::chrono::hh_mm_ss<std::chrono::seconds> Time;
};

struct Schedule
{
    std::string Name;
    bool Enabled;
    std::vector<ScheduleEntry> Entries;
};

#endif // INCLUDE_CORE_SCHEDULE_H_
