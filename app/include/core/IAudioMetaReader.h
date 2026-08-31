#ifndef INCLUDE_CORE_IAUDIOMETAREADER_H_
#define INCLUDE_CORE_IAUDIOMETAREADER_H_

#include "core/Audio.h"
#include <span>

class IAudioMetaReader
{
public:
    virtual ~IAudioMetaReader() = default;

    virtual AudioMeta ReadMeta(std::span<const std::byte> audio) = 0;
};

#endif // INCLUDE_CORE_IAUDIOMETAREADER_H_
