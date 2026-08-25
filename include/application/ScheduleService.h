#ifndef INCLUDE_APPLICATION_SCHEDULESERVICE_H_
#define INCLUDE_APPLICATION_SCHEDULESERVICE_H_

#include "core/Schedule.h"
#include "core/IStorage.h"
#include "core/Enums.h"
#include "core/IAudioStorage.h"
#include "application/AudioPlayerController.h"
#include <thread>
#include <condition_variable>
#include <exception>

class ScheduleServiceException : public std::exception
{
public:
    ScheduleServiceException(ScheduleServiceErrorCode code, const std::string& msg);
    const char* what() const noexcept override;
    ScheduleServiceErrorCode code() const noexcept;

private:
    std::string m_Message;
    ScheduleServiceErrorCode m_Code;
};

class ScheduleService
{
public:
    ScheduleService(IStorage<std::string, Schedule>& schedule_storage, AudioPlayerController& player_controller, IAudioStorage& audio_storage);
    ~ScheduleService();

    void AddSchedule(const Schedule& schedule);
    void UpdateSchedule(const Schedule& schedule);
    void RemoveSchedule(const std::string& name);
    void SetScheduleState(const std::string& name, bool enabled);

private:
    IStorage<std::string, Schedule>& m_ScheduleStorage;
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

#endif // INCLUDE_APPLICATION_SCHEDULESERVICE_H_