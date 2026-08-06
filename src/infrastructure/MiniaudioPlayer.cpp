#include "infrastructure/MiniaudioPlayer.h"
#include <stdexcept>

MiniaudioPlayer::MiniaudioPlayer() :
    m_WakeUp(false),
    m_Worker(&MiniaudioPlayer::PostSoundFinished, this)
{
    if(ma_engine_init(nullptr, &m_Engine) != MA_SUCCESS)
        throw std::runtime_error("Failed to init miniaudio engine");
}

MiniaudioPlayer::~MiniaudioPlayer()
{
    Stop();
    ma_engine_uninit(&m_Engine);
}

void MiniaudioPlayer::Play(const Audio& audio)
{
    Stop();

    m_CurrentAudio = audio;

    ma_result result = ma_decoder_init_memory(m_CurrentAudio.Data.data(), m_CurrentAudio.Data.size(), nullptr, &m_Decoder);
    if(result != MA_SUCCESS)
    {
        throw std::runtime_error("Failed to initialize decoder");
    }

    result = ma_sound_init_from_data_source(&m_Engine, &m_Decoder, 0, nullptr, &m_Sound);
    if(result != MA_SUCCESS)
    {
        ma_decoder_uninit(&m_Decoder);
        throw std::runtime_error("Failed to initialize sound");
    }

    ma_sound_set_end_callback(&m_Sound, PlaybackFinishedCallback, this);
    result = ma_sound_start(&m_Sound);
    if(result != MA_SUCCESS)
    {
        ma_sound_uninit(&m_Sound);
        ma_decoder_uninit(&m_Decoder);
        throw std::runtime_error("Failed to start sound");
    }

    m_IsInitialized = true;
}

void MiniaudioPlayer::Stop()
{
    if(!m_IsInitialized) return;
    ma_sound_stop(&m_Sound);
    ma_sound_uninit(&m_Sound);
    ma_decoder_uninit(&m_Decoder);
    m_IsInitialized = false;
}

bool MiniaudioPlayer::IsPlaying()
{
    if(!m_IsInitialized) return false;
    return ma_sound_is_playing(&m_Sound);
}

void MiniaudioPlayer::SetVolume(float volume)
{
    if(ma_engine_set_volume(&m_Engine, volume))
        throw std::runtime_error("Failed to set volume");
}

float MiniaudioPlayer::GetVolume()
{
    return ma_engine_get_volume(&m_Engine);
}

void MiniaudioPlayer::Subscribe(IAudioPlayerObserver* obs)
{
    m_Observers.push_back(obs);
}

void MiniaudioPlayer::PostSoundFinished()
{
    while(true)
    {
        std::unique_lock lock(m_Mutex);
        m_Cv.wait(lock, [&]() {
            return m_WakeUp || m_Stop;
        });

        if(m_Stop) break;

        Stop();
        m_WakeUp = false;
        for(auto obs : m_Observers)
        {
            obs->OnPlaybackFinished();
        }
    }
}

void MiniaudioPlayer::PlaybackFinishedCallback(void* pUserData, ma_sound* pSound)
{
    auto* player = static_cast<MiniaudioPlayer*>(pUserData);
    {
        std::lock_guard lock(player->m_Mutex);
        player->m_WakeUp = true;
    }
    player->m_Cv.notify_one();
}
