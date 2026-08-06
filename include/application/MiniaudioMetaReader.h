#ifndef INCLUDE_APPLICATION_MINIAUDIOMETAREADER_H_
#define INCLUDE_APPLICATION_MINIAUDIOMETAREADER_H_

#include "core/IAudioMetaReader.h"

class MiniaudioMetaReader : public IAudioMetaReader
{
public:
    AudioMeta ReadMeta(std::span<const std::byte> audio) override;
};

#endif // INCLUDE_APPLICATION_MINIAUDIOMETAREADER_H_
