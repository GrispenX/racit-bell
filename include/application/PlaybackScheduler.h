#ifndef INCLUDE_APPLICATION_PLAYBACKSCHEDULER_H_
#define INCLUDE_APPLICATION_PLAYBACKSCHEDULER_H_

#include "core/IAudioStorage.h"
#include "infrastructure/InMemStorage.h"
#include "application/AudioPlayerController.h"
#include <mutex>
#include <condition_variable>
#include <thread>

class PlaybackScheduler
{
public:
    PlaybackScheduler(AudioPlayerController& audioplayer_controller, IAudioStorage& audio_storage);
    ~PlaybackScheduler();

    void Schedule(const Playback& playback);
    void Unschedule(int id);
    std::vector<Playback> GetScheduledPlaybacks();

private:
    IAudioStorage& m_AudioStorage;
    AudioPlayerController& m_AudioPlayerController;
    InMemStorage<Playback> m_Playbacks;

    std::mutex m_PbMutex;
    std::mutex m_WorkerMutex;
    std::condition_variable m_Cv;
    std::thread m_Worker;
    bool m_IsRunning;
    bool m_Reschedule;

    void Worker();
};

#endif // INCLUDE_APPLICATION_PLAYBACKSCHEDULER_H_
