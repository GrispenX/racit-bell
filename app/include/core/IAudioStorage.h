#ifndef INCLUDE_CORE_IAUDIOSTORAGE_H_
#define INCLUDE_CORE_IAUDIOSTORAGE_H_

#include "core/Audio.h"
#include <optional>
#include <vector>

class IAudioStorage
{
public:
    virtual ~IAudioStorage() = default;

    virtual void Add(Audio& audio) = 0;
    virtual void Remove(std::string name) = 0;
    virtual bool Exists(std::string name) = 0;
    virtual Audio Get(std::string name) = 0;
    virtual std::vector<AudioMeta> List() = 0;
};

#endif // INCLUDE_CORE_IAUDIOSTORAGE_H_
