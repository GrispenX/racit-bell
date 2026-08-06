#ifndef INCLUDE_CORE_IAUDIOPLAYER_H_
#define INCLUDE_CORE_IAUDIOPLAYER_H_

#include "core/Audio.h"
#include "core/IAudioPlayerObserver.h"

class IAudioPlayer
{
public:
    virtual ~IAudioPlayer() = default;

    virtual void Play(const Audio& audio) = 0;
    virtual void Stop() = 0;
    virtual bool IsPlaying() = 0;
    virtual void SetVolume(float volume) = 0;
    virtual float GetVolume() = 0;

    virtual void Subscribe(IAudioPlayerObserver* obs) = 0;
};

#endif // INCLUDE_CORE_IAUDIOPLAYER_H_
