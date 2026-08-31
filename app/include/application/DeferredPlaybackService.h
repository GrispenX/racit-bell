#ifndef INCLUDE_APPLICATION_DEFERREDPLAYBACKSERVICE_H_
#define INCLUDE_APPLICATION_DEFERREDPLAYBACKSERVICE_H_

#include "core/IStorage.h"
#include "core/DeferredPlayback.h"
#include "core/IAudioStorage.h"
#include "core/Enums.h"
#include "application/AudioPlayerController.h"
#include <thread>
#include <condition_variable>
#include <exception>

class DeferredPlaybackServiceException : public std::exception
{
public:
    DeferredPlaybackServiceException(DeferredPlaybackServiceErrorCode code, const std::string& msg);
    const char* what() const noexcept override;
    DeferredPlaybackServiceErrorCode code() const noexcept;

private:
    std::string m_Message;
    DeferredPlaybackServiceErrorCode m_Code;
};

class DeferredPlaybackService
{
public:
    DeferredPlaybackService(IStorage<int, DeferredPlayback>& deferred_pb_storage, AudioPlayerController& player_controller, IAudioStorage& audio_storage);
    ~DeferredPlaybackService();

    void Schedule(const DeferredPlayback& deferred_pb);
    void Unschedule(int id);
    std::vector<DeferredPlayback> GetDeferredPlaybacks();

private:
    IStorage<int, DeferredPlayback>& m_PbStorage;
    AudioPlayerController& m_PlayerController;
    IAudioStorage& m_AudioStorage;

    std::mutex m_Mutex;
    std::mutex m_WorkerMutex;
    std::condition_variable m_Cv;
    std::thread m_Worker;
    bool m_IsRunning;
    bool m_Reschedule;

    void Worker();
};

#endif // INCLUDE_APPLICATION_DEFERREDPLAYBACKSERVICE_H_
