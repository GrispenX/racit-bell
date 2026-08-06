#ifndef INCLUDE_APPLICATION_AUDIOPLAYERCONTROLLER_H_
#define INCLUDE_APPLICATION_AUDIOPLAYERCONTROLLER_H_

#include "core/IAudioPlayer.h"
#include "core/IAudioStorage.h"
#include "core/IAudioPlayerObserver.h"
#include "core/Playback.h"
#include "infrastructure/InMemStorage.h"
#include <mutex>
#include <memory>

class AudioPlayerController : public IAudioPlayerObserver
{
public:
    AudioPlayerController(std::unique_ptr<IAudioPlayer> player, IAudioStorage& audio_storage);

    void Enqueue(const Playback& playback);
    void Dequeue(int id);
    std::vector<Playback> GetQueuedPlaybacks();
    void StopCurrent();
    void SetVolume(float volume);
    float GetVolume();

    std::vector<AudioMeta> GetAvaliableAudios();

    void OnPlaybackFinished() override;

private:
    std::optional<Playback> GetNextPlayback();
    void StartPlayback(const Playback& playback);

    std::unique_ptr<IAudioPlayer> m_AudioPlayer;
    IAudioStorage& m_AudioStorage;
    InMemStorage<Playback> m_Queue;

    std::mutex m_Mutex;
    Playback m_CurrentPlayback;
};

#endif // INCLUDE_APPLICATION_AUDIOPLAYERCONTROLLER_H_
