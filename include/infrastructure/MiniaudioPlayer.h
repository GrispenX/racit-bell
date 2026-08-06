#ifndef INCLUDE_INFRASTRUCTURE_MINIAUDIOPLAYER_H_
#define INCLUDE_INFRASTRUCTURE_MINIAUDIOPLAYER_H_

#include "core/IAudioPlayer.h"
#include <vector>
#include <miniaudio/miniaudio.h>
#include <thread>
#include <condition_variable>

class MiniaudioPlayer : public IAudioPlayer
{
public:
    MiniaudioPlayer();
    ~MiniaudioPlayer() override;

    void Play(const Audio& audio) override;
    void Stop() override;
    bool IsPlaying() override;
    void SetVolume(float volume) override;
    float GetVolume() override;

    void Subscribe(IAudioPlayerObserver* obs) override;

private:
    bool m_IsInitialized = false;
    ma_engine m_Engine;
    ma_decoder m_Decoder;
    ma_sound m_Sound;
    Audio m_CurrentAudio;

    std::vector<IAudioPlayerObserver*> m_Observers;
    std::thread m_Worker;
    std::condition_variable m_Cv;
    std::mutex m_Mutex;
    bool m_WakeUp;
    bool m_Stop;

    void PostSoundFinished();
    static void PlaybackFinishedCallback(void* pUserData, ma_sound* pSound);
};

#endif // INCLUDE_INFRASTRUCTURE_MINIAUDIOPLAYER_H_
