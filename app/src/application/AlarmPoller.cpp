#include "application/AlarmPoller.h"
#include "httplib.h"

AlarmPoller::AlarmPoller(std::string api_key, AudioPlayerController& player_controller) :
    m_ApiKey(api_key),
    m_AudioPlayerController(player_controller)
{
    m_Worker = std::thread(&AlarmPoller::Worker, this);
}

void AlarmPoller::Worker()
{
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(15));
        httplib::SSLClient client("api.ukrainealarm.com");
        auto res = client.Get("/api/v3/alerts/" + m_RegionId, {{"Authorization", m_ApiKey}});
        if(!res) continue;
    }
}
