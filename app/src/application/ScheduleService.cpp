#include "application/ScheduleService.h"
#include <optional>
#include <format>

ScheduleServiceException::ScheduleServiceException(ScheduleServiceErrorCode code, const std::string& msg) :
    m_Code(code),
    m_Message(msg)
{}

const char* ScheduleServiceException::what() const noexcept
{
    return m_Message.c_str();
}

ScheduleServiceErrorCode ScheduleServiceException::code() const noexcept
{
    return m_Code;
}


ScheduleService::ScheduleService(IStorage<std::string, Schedule>& schedule_storage, AudioPlayerController& player_controller, IAudioStorage& audio_storage) :
    m_ScheduleStorage(schedule_storage),
    m_PlayerController(player_controller),
    m_AudioStorage(audio_storage),
    m_IsRunning(true),
    m_Reschedule(false)
{
    m_Worker = std::thread(&ScheduleService::Worker, this);
}

ScheduleService::~ScheduleService()
{
    {
        std::lock_guard lock(m_WorkerMutex);
        m_Reschedule = true;
    }
    m_Cv.notify_one();
    if(m_Worker.joinable()) m_Worker.join();
}


void ScheduleService::AddSchedule(const Schedule& schedule)
{
    std::lock_guard lock(m_Mutex);

    if(schedule.Entries.empty())
    {
        throw ScheduleServiceException(
            ScheduleServiceErrorCode::SCHEDULE_EMPTY,
            "Schedule has no entries."
        );
    }

    if(m_ScheduleStorage.Exists(schedule.Name))
    {
        throw ScheduleServiceException(
            ScheduleServiceErrorCode::SCHEDULE_NAME_ALREADY_TAKEN,
            std::format("Schedule with name '{}' already exists.", schedule.Name)
        );
    }

    for(const ScheduleEntry& entry : schedule.Entries)
    {
        if(!m_AudioStorage.Exists(entry.AudioName))
        {
            throw ScheduleServiceException(
                ScheduleServiceErrorCode::INVALID_AUDIO_NAME,
                std::format("Audio with name '{}' does not exist.", entry.AudioName)
            );
        }
    }

    m_ScheduleStorage.Add(schedule);
    {
        std::lock_guard lock(m_WorkerMutex);
        m_Reschedule = true;
    }
    m_Cv.notify_one();
}

void ScheduleService::UpdateSchedule(const Schedule& schedule)
{
    std::lock_guard lock(m_Mutex);

    if(schedule.Entries.empty())
    {
        throw ScheduleServiceException(
            ScheduleServiceErrorCode::SCHEDULE_EMPTY,
            "Schedule has no entries."
        );
    }

    for(const ScheduleEntry& entry : schedule.Entries)
    {
        if(!m_AudioStorage.Exists(entry.AudioName))
        {
            throw ScheduleServiceException(
                ScheduleServiceErrorCode::INVALID_AUDIO_NAME,
                std::format("Audio with name '{}' does not exist.", entry.AudioName)
            );
        }
    }

    try
    {
        m_ScheduleStorage.Update(schedule);
    }
    catch(const StorageException& e)
    {
        throw ScheduleServiceException(
            ScheduleServiceErrorCode::INVALID_SCHEDULE_NAME,
            std::format("Schedule with name '{}' does not exist.", schedule.Name)
        );
    }

    {
        std::lock_guard lock(m_WorkerMutex);
        m_Reschedule = true;
    }
    m_Cv.notify_one();
}

void ScheduleService::RemoveSchedule(const std::string& name)
{
    std::lock_guard lock(m_Mutex);

    try
    {
        m_ScheduleStorage.Remove(name);
    }
    catch(const StorageException& e)
    {
        throw ScheduleServiceException(
            ScheduleServiceErrorCode::INVALID_SCHEDULE_NAME,
            std::format("Schedule with name '{}' does not exist.", name)
        );
    }
    
    {
        std::lock_guard lock(m_WorkerMutex);
        m_Reschedule = true;
    }
    m_Cv.notify_one();
}

void ScheduleService::SetScheduleState(const std::string& name, bool enabled)
{
    std::lock_guard lock(m_Mutex);
    
    try
    {
        Schedule schedule = m_ScheduleStorage.Get(name);
        schedule.Enabled = enabled;
        m_ScheduleStorage.Update(schedule);
    }
    catch(const StorageException& e)
    {
        throw ScheduleServiceException(
            ScheduleServiceErrorCode::INVALID_SCHEDULE_NAME,
            std::format("Schedule with name '{}' does not exist.", name)
        );
    }

    {
        std::lock_guard lock(m_WorkerMutex);
        m_Reschedule = true;
    }
    m_Cv.notify_one();
}


void ScheduleService::Worker()
{
    while(true)
    {
        std::vector<Schedule> active_schedules = m_ScheduleStorage.Get([](const Schedule& s) {
            return s.Enabled;
        });

        std::vector<ScheduleEntry> active_entries;
        for(const Schedule& s : active_schedules)
        {
            for(const ScheduleEntry& e : s.Entries)
            {
                active_entries.push_back(e);
            }
        }

        ScheduleEntry next_entry;
        std::optional<std::chrono::system_clock::time_point> next_time;

        {
            std::unique_lock lock(m_WorkerMutex);
            if(active_entries.empty())
            {
                m_Cv.wait(lock, [&]() {
                    return !m_IsRunning || m_Reschedule;
                });
            }
            else
            {
                const auto now = std::chrono::system_clock::now();
                const auto today = std::chrono::floor<std::chrono::days>(now);
                for(const ScheduleEntry& entry : active_entries)
                {
                    auto entry_next_time = std::chrono::system_clock::time_point(today) + entry.Time.to_duration();
                    if(entry_next_time <= now) entry_next_time += std::chrono::days(1);
                    if(!next_time || entry_next_time < *next_time)
                    {
                        next_time = entry_next_time;
                        next_entry = entry;
                    }
                }
                m_Cv.wait_until(lock, *next_time, [&]() {
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

        try
        {
            m_PlayerController.Enqueue({
                .AudioName = next_entry.AudioName,
                .Priority = next_entry.Priority
            });
        }
        catch(const AudioPlayerControllerException& e) {}
    }
}
