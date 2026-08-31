#ifndef INCLUDE_CORE_ISTORAGE_H_
#define INCLUDE_CORE_ISTORAGE_H_

#include <vector>
#include <functional>
#include <exception>
#include <string>

class StorageException : public std::exception
{
public:
    StorageException(const std::string& msg);
    const char* what() const noexcept override;

private:
    std::string m_Message;
};

template<typename TKey, typename TVal>
class IStorage
{
public:
    virtual ~IStorage() = default;

    virtual TKey Add(const TVal& value) = 0;
    virtual void Update(const TVal& value) = 0;
    virtual void Remove(const TKey& key) = 0;
    virtual bool Exists(const TKey& key) = 0;
    virtual TVal Get(const TKey& key) = 0;
    virtual std::vector<TVal> Get(std::function<bool(const TVal&)> predicate) = 0;
    virtual std::vector<TVal> Get() = 0;
};

#endif // INCLUDE_CORE_ISTORAGE_H_
