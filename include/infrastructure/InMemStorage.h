#ifndef INCLUDE_INFRASTRUCTURE_INMEMSTORAGE_H_
#define INCLUDE_INFRASTRUCTURE_INMEMSTORAGE_H_

#include "core/IStorage.h"
#include <unordered_map>
#include <stdexcept>

template<typename T>
class InMemStorage : public IStorage<int, T>
{
public:
    int Add(const T& value) override
    {
        int id = m_NextID++;
        m_Data.insert({id, value});
        m_Data[id].Id = id;
        return id;
    }

    void Update(const T& value) override
    {
        auto it = m_Data.find(value.Id);
        if(it == m_Data.end()) throw std::runtime_error("Value not found");
        it->second = value;
    }

    void Remove(int id) override
    {
        m_Data.erase(id);
    }

    std::optional<T> Get(int id) override
    {
        auto it = m_Data.find(id);
        if(it == m_Data.end()) return std::nullopt;
        return it->second;
    }

    std::vector<T> Get(std::function<bool(const T&)> predicate) override
    {
        std::vector<T> suitable;
        for(auto& [_, value] : m_Data)
        {
            if(predicate(value)) suitable.push_back(value);
        }
        return suitable;
    }

    std::vector<T> Get() override
    {
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
};


#endif // INCLUDE_INFRASTRUCTURE_INMEMSTORAGE_H_
