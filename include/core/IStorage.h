#ifndef INCLUDE_CORE_ISTORAGE_H_
#define INCLUDE_CORE_ISTORAGE_H_

#include <optional>
#include <vector>
#include <functional>

template<typename TKey, typename TVal>
class IStorage
{
public:
    virtual ~IStorage() = default;

    virtual TKey Add(const TVal& value) = 0;
    virtual void Update(const TVal& value) = 0;
    virtual void Remove(int id) = 0;
    virtual std::optional<TVal> Get(int id) = 0;
    virtual std::vector<TVal> Get(std::function<bool(const TVal&)> predicate) = 0;
    virtual std::vector<TVal> Get() = 0;
};

#endif // INCLUDE_CORE_ISTORAGE_H_
