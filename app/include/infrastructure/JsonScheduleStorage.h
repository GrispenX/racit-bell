#ifndef INCLUDE_INFRASTRUCTURE_JSONSCHEDULESTORAGE_H_
#define INCLUDE_INFRASTRUCTURE_JSONSCHEDULESTORAGE_H_

#include "core/IStorage.h"
#include "core/Schedule.h"
#include <filesystem>
#include <mutex>

class JsonScheduleStorage : public IStorage<std::string, Schedule>
{
public:
    JsonScheduleStorage(std::filesystem::path path);
    
    std::string Add(const Schedule& schedule) override;
    void Update(const Schedule& schedule) override;
    void Remove(const std::string& name) override;
    bool Exists(const std::string& name) override;
    Schedule Get(const std::string& name) override;
    std::vector<Schedule> Get(std::function<bool(const Schedule&)> predicate) override;
    std::vector<Schedule> Get() override;


private:
    std::filesystem::path m_Path;
    std::vector<Schedule> m_Schedules;
    std::mutex m_Mutex;
    void Save();
};

#endif // INCLUDE_INFRASTRUCTURE_JSONSCHEDULESTORAGE_H_
