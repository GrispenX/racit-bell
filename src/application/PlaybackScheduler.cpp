#include "application/PlaybackScheduler.h"
#include <algorithm>
#include <stdexcept>
#include <format>

PlaybackScheduler::PlaybackScheduler(AudioPlayerController& audioplayer_controller, IAudioStorage& audio_storage):
    m_AudioPlayerController(audioplayer_controller),
    m_AudioStorage(audio_storage),
    m_IsRunning(true)
{
    m_Worker = std::thread(&PlaybackScheduler::Worker, this);
}

PlaybackScheduler::~PlaybackScheduler()
{
    {
        std::lock_guard lock(m_WorkerMutex);
        m_IsRunning = false;
    }
    m_Cv.notify_one();
    if(m_Worker.joinable()) m_Worker.join();
}



void PlaybackScheduler::Schedule(const Playback& playback)
{
    if(!m_AudioStorage.Exists(playback.AudioName))
        throw std::runtime_error(std::format("Audio with name {} does not exist.", playback.AudioName));
    {
        std::lock_guard lock(m_PbMutex);
        m_Playbacks.Add(playback);
    }
    {
        std::lock_guard lock(m_WorkerMutex);
        m_Reschedule = true;
    }
    m_Cv.notify_one();
}

void PlaybackScheduler::Unschedule(int id)
{
    {
        std::lock_guard lock(m_PbMutex);
        m_Playbacks.Remove(id);
    }
    {
        std::lock_guard lock(m_WorkerMutex);
        m_Reschedule = true;
    }
    m_Cv.notify_one();
}

std::vector<Playback> PlaybackScheduler::GetScheduledPlaybacks()
{
    std::lock_guard lock(m_PbMutex);
    return m_Playbacks.Get();
}



void PlaybackScheduler::Worker()
{
    while(true)
    {
        std::vector<Playback> playbacks;
        {
            std::lock_guard lock(m_PbMutex);
            playbacks = m_Playbacks.Get();
        }
        {
            std::unique_lock lock(m_WorkerMutex);
            if(playbacks.empty())
            {
                m_Cv.wait(lock, [&]() {
                    return !m_IsRunning || m_Reschedule;
                });
            }
            else
            {
                auto next_time = std::min_element(playbacks.begin(), playbacks.end(), [](const Playback& prev, const Playback& curr) {
                    return prev.Time < curr.Time;
                })->Time;
                m_Cv.wait_until(lock, next_time, [&]() {
                    return !m_IsRunning || m_Reschedule;
                });
            }

            if(!m_IsRunning) break;
            if(m_Reschedule)
            {
                m_Reschedule = false;
                continue;
            }
        }

        std::lock_guard lock(m_PbMutex);

        auto now = std::chrono::system_clock::now();
        std::vector<Playback> to_play = m_Playbacks.Get([&](const Playback& pb) {
            return pb.Time <= now;
        });
        for(const auto& playback : to_play)
        {
            m_Playbacks.Remove(playback.Id);
            try
            {
                m_AudioPlayerController.Enqueue(playback);
            }
            catch(const std::exception& e) {}
        }
    }
}
