#include "application/DeferredPlaybackService.h"
#include <algorithm>
#include <format>

DeferredPlaybackServiceException::DeferredPlaybackServiceException(DeferredPlaybackServiceErrorCode code, const std::string& msg) :
    m_Code(code),
    m_Message(msg)
{}

const char* DeferredPlaybackServiceException::what() const noexcept
{
    return m_Message.c_str();
}

DeferredPlaybackServiceErrorCode DeferredPlaybackServiceException::code() const noexcept
{
    return m_Code;
}


DeferredPlaybackService::DeferredPlaybackService(IStorage<int, DeferredPlayback>& deferred_pb_storage, AudioPlayerController& player_controller, IAudioStorage& audio_storage):
    m_PbStorage(deferred_pb_storage),
    m_PlayerController(player_controller),
    m_AudioStorage(audio_storage),
    m_IsRunning(true),
    m_Reschedule(false)
{
    m_Worker = std::thread(&DeferredPlaybackService::Worker, this);
}

DeferredPlaybackService::~DeferredPlaybackService()
{
    {
        std::lock_guard lock(m_WorkerMutex);
        m_IsRunning = false;
    }
    m_Cv.notify_one();
    if(m_Worker.joinable()) m_Worker.join();
}


void DeferredPlaybackService::Schedule(const DeferredPlayback& pb)
{
    std::lock_guard lock(m_Mutex);
    if(!m_AudioStorage.Exists(pb.AudioName))
    {
        throw DeferredPlaybackServiceException(
            DeferredPlaybackServiceErrorCode::INVALID_AUDIO_NAME,
            std::format("Audio with name '{}' does not exist.", pb.AudioName)
        );
    }
    m_PbStorage.Add(pb);
    {
        std::lock_guard lock(m_WorkerMutex);
        m_Reschedule = true;
    }
    m_Cv.notify_one();
}

void DeferredPlaybackService::Unschedule(int id)
{
    std::lock_guard lock(m_Mutex);

    try
    {
        m_PbStorage.Remove(id);
    }
    catch(const StorageException& e)
    {
        throw DeferredPlaybackServiceException(
            DeferredPlaybackServiceErrorCode::INVALID_ID,
            std::format("Deferred playback with id '{}' does not exist.", id)
        );
    }
    
    {
        std::lock_guard lock(m_WorkerMutex);
        m_Reschedule = true;
    }
    m_Cv.notify_one();
}

std::vector<DeferredPlayback> DeferredPlaybackService::GetDeferredPlaybacks()
{
    std::lock_guard lock(m_Mutex);
    return m_PbStorage.Get();
}



void DeferredPlaybackService::Worker()
{
    while(true)
    {
        std::vector<DeferredPlayback> playbacks = m_PbStorage.Get();
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
                auto next_time = std::min_element(playbacks.begin(), playbacks.end(), [](const DeferredPlayback& prev, const DeferredPlayback& curr) {
                    return prev.PlayTime < curr.PlayTime;
                })->PlayTime;
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

        auto now = std::chrono::system_clock::now();
        std::vector<DeferredPlayback> to_play = m_PbStorage.Get([&](const DeferredPlayback& pb) {
            return pb.PlayTime <= now;
        });
        for(const DeferredPlayback& pb : to_play)
        {
            m_PbStorage.Remove(pb.Id);
            try
            {
                m_PlayerController.Enqueue({
                    .AudioName = pb.AudioName,
                    .Priority = pb.Priority
                });
            }
            catch(const AudioPlayerControllerException& e) {}
        }
    }
}
