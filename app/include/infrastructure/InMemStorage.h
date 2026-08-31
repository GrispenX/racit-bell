#ifndef INCLUDE_INFRASTRUCTURE_INMEMSTORAGE_H_
#define INCLUDE_INFRASTRUCTURE_INMEMSTORAGE_H_

#include "core/IStorage.h"
#include <unordered_map>
#include <format>
#include <mutex>

template<typename T>
class InMemStorage : public IStorage<int, T>
{
public:
    int Add(const T& value) override
    {
        std::lock_guard lock(m_Mutex);
        int id = m_NextID++;
        m_Data.insert({id, value});
        m_Data[id].Id = id;
        return id;
    }

    void Update(const T& value) override
    {
        std::lock_guard lock(m_Mutex);
        auto it = m_Data.find(value.Id);
        if(it == m_Data.end())
        {
            throw StorageException(std::format("Value with id '{}' does not exist.", value.Id));
        }
        it->second = value;
    }

    void Remove(const int& id) override
    {
        std::lock_guard lock(m_Mutex);
        if(m_Data.find(id) == m_Data.end())
        {
            throw StorageException(std::format("Value with id '{}' does not exist.", id));
        }
        m_Data.erase(id);
    }

    bool Exists(const int& id) override
    {
        std::lock_guard lock(m_Mutex);
        return m_Data.find(id) != m_Data.end();
    }

    T Get(const int& id) override
    {
        std::lock_guard lock(m_Mutex);
        auto it = m_Data.find(id);
        if(it == m_Data.end())
        {
            throw StorageException(std::format("Value with id '{}' does not exist.", id));
        }
        return it->second;
    }

    std::vector<T> Get(std::function<bool(const T&)> predicate) override
    {
        std::lock_guard lock(m_Mutex);
        std::vector<T> suitable;
        for(auto& [_, value] : m_Data)
        {
            if(predicate(value)) suitable.push_back(value);
        }
        return suitable;
    }

    std::vector<T> Get() override
    {
        std::lock_guard lock(m_Mutex);
        std::vector<T> values;
        for(auto& [_, value] : m_Data)
        {
            values.push_back(value);
        }
        return values;
    }

private:
    int m_NextID = 1;
    std::unordered_map<int, T> m_Data;

    std::mutex m_Mutex;
};


#endif // INCLUDE_INFRASTRUCTURE_INMEMSTORAGE_H_
