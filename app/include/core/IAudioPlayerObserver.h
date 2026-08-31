#ifndef INCLUDE_CORE_IAUDIOPLAYEROBSERVER_H_
#define INCLUDE_CORE_IAUDIOPLAYEROBSERVER_H_

class IAudioPlayerObserver
{
public:
    virtual ~IAudioPlayerObserver() = default;

    virtual void OnPlaybackFinished() = 0;
};

#endif // INCLUDE_CORE_IAUDIOPLAYEROBSERVER_H_
