#ifndef INCLUDE_APPLICATION_ALARMPOLLER_H_
#define INCLUDE_APPLICATION_ALARMPOLLER_H_

#include "application/AudioPlayerController.h"
#include <thread>
#include <chrono>

class AlarmPoller
{
public:
    AlarmPoller(std::string api_key, AudioPlayerController& player_controller);

private:
    std::string m_ApiKey;
    AudioPlayerController& m_AudioPlayerController;
    std::string m_RegionId;

    std::thread m_Worker;
    void Worker();
};

#endif // APP_INCLUDE_APPLICATION_ALARMPOLLER_H_
