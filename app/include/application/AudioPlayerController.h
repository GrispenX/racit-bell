#ifndef INCLUDE_APPLICATION_AUDIOPLAYERCONTROLLER_H_
#define INCLUDE_APPLICATION_AUDIOPLAYERCONTROLLER_H_

#include "core/IAudioPlayer.h"
#include "core/IAudioStorage.h"
#include "core/IAudioPlayerObserver.h"
#include "core/IStorage.h"
#include "core/QueuedPlayback.h"
#include "core/Enums.h"
#include <mutex>
#include <memory>
#include <exception>

class AudioPlayerControllerException : public std::exception
{
public:
    AudioPlayerControllerException(AudioPlayerControllerErrorCode code, const std::string& msg);
    const char* what() const noexcept override;
    AudioPlayerControllerErrorCode code() const noexcept;

private:
    std::string m_Message;
    AudioPlayerControllerErrorCode m_Code;
};

class AudioPlayerController : public IAudioPlayerObserver
{
public:
    AudioPlayerController(std::unique_ptr<IAudioPlayer> player, IAudioStorage& audio_storage, IStorage<int, QueuedPlayback>& queued_pb_storage);

    void Enqueue(const QueuedPlayback& queued_pb);
    void Dequeue(int id);
    std::vector<QueuedPlayback> GetQueuedPlaybacks();
    void StopCurrent();
    void SetVolume(float volume);
    float GetVolume();

    void OnPlaybackFinished() override;

private:
    std::optional<QueuedPlayback> GetNextPlayback();
    void StartPlayback(const QueuedPlayback& pb);

    std::unique_ptr<IAudioPlayer> m_AudioPlayer;
    IAudioStorage& m_AudioStorage;
    IStorage<int, QueuedPlayback>& m_Queue;
    QueuedPlayback m_CurrentPb;

    std::mutex m_Mutex;
};

#endif // INCLUDE_APPLICATION_AUDIOPLAYERCONTROLLER_H_
