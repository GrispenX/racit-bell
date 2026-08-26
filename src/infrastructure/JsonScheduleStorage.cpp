#include "infrastructure/JsonScheduleStorage.h"
#include "core/JsonSerialization.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <format>
#include <algorithm>

JsonScheduleStorage::JsonScheduleStorage(std::filesystem::path path) :
    m_Path(path)
{
    if(std::filesystem::exists(path))
    {
        std::ifstream ifile(path);
        nlohmann::json j = nlohmann::json::parse(ifile);
        for(const auto& schedule : j)
        {
            m_Schedules.push_back(schedule);
        }
    }
}


std::string JsonScheduleStorage::Add(const Schedule& schedule)
{
    std::lock_guard lock(m_Mutex);

    for(const auto& s : m_Schedules)
    {
        if(s.Name == schedule.Name)
        {
            throw StorageException(std::format("Schedule with name {} already exists.", schedule.Name));
        }
    }
    m_Schedules.push_back(schedule);
    Save();
    return schedule.Name;
}

void JsonScheduleStorage::Update(const Schedule& schedule)
{
    std::lock_guard lock(m_Mutex);

    const auto it = std::find_if(m_Schedules.begin(), m_Schedules.end(), [&](const Schedule& s) {
        return s.Name == schedule.Name;
    });
    if(it == m_Schedules.end())
    {
        throw StorageException(std::format("Schedule with name {} does not exist.", schedule.Name));
    }
    *it = schedule;
    Save();
}

void JsonScheduleStorage::Remove(const std::string& name)
{
    std::lock_guard lock(m_Mutex);

    const auto it = std::find_if(m_Schedules.begin(), m_Schedules.end(), [&](const Schedule& s) {
        return s.Name == name;
    });
    if(it == m_Schedules.end())
    {
        throw StorageException(std::format("Schedule with name {} does not exist.", name));
    }
    m_Schedules.erase(it);
    Save();
}

bool JsonScheduleStorage::Exists(const std::string& name)
{
    std::lock_guard lock(m_Mutex);

    const auto it = std::find_if(m_Schedules.begin(), m_Schedules.end(), [&](const Schedule& s) {
        return s.Name == name;
    });
    return it != m_Schedules.end();
}

Schedule JsonScheduleStorage::Get(const std::string& name)
{
    std::lock_guard lock(m_Mutex);

    const auto it = std::find_if(m_Schedules.begin(), m_Schedules.end(), [&](const Schedule& s) {
        return s.Name == name;
    });
    if(it == m_Schedules.end())
    {
        throw StorageException(std::format("Schedule with name {} does not exist.", name));
    }
    return *it;
}

std::vector<Schedule> JsonScheduleStorage::Get(std::function<bool(const Schedule&)> predicate)
{
    std::lock_guard lock(m_Mutex);

    std::vector<Schedule> suitable;
    for(const auto& s : m_Schedules)
    {
        if(predicate(s))
        {
            suitable.push_back(s);
        }
    }
    return suitable;
}

std::vector<Schedule> JsonScheduleStorage::Get()
{
    std::lock_guard lock(m_Mutex);

    return m_Schedules;
}


void JsonScheduleStorage::Save()
{
    nlohmann::json j = m_Schedules;
    std::ofstream ofile(m_Path);
    ofile << j.dump(4) << '\n';
    ofile.close();
}
