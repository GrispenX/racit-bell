#include "application/AudioPlayerController.h"
#include <algorithm>
#include <stdexcept>
#include <format>

AudioPlayerController::AudioPlayerController(std::unique_ptr<IAudioPlayer> audio_player, IAudioStorage& audio_storage) :
    m_AudioPlayer(std::move(audio_player)),
    m_AudioStorage(audio_storage)
{
    m_AudioPlayer->Subscribe(this);
}


void AudioPlayerController::Enqueue(const Playback& playback)
{
    std::lock_guard lock(m_Mutex);
    
    if(!m_AudioStorage.Exists(playback.AudioName))
        throw std::runtime_error(std::format("Audio with name {} does not exist.", playback.AudioName));

    if(!m_AudioPlayer->IsPlaying())
    {
        StartPlayback(playback);
    }
    else if(m_CurrentPlayback.Priority < playback.Priority)
    {
        m_AudioPlayer->Stop();
        StartPlayback(playback);
    }
    else
    {
        Playback pb = playback;
        pb.Time = std::chrono::system_clock::now();
        m_Queue.Add(pb);
    }
}

void AudioPlayerController::Dequeue(int id)
{
    std::lock_guard lock(m_Mutex);
    m_Queue.Remove(id);
}

std::vector<Playback> AudioPlayerController::GetQueuedPlaybacks()
{
    std::lock_guard lock(m_Mutex);
    return m_Queue.Get();
}

void AudioPlayerController::StopCurrent()
{
    std::lock_guard lock(m_Mutex);
    m_AudioPlayer->Stop();
    std::optional<Playback> next_pb = GetNextPlayback();
    if(!next_pb.has_value()) return;
    m_Queue.Remove(next_pb->Id);
    StartPlayback(*next_pb);
}

void AudioPlayerController::SetVolume(float volume)
{
    m_AudioPlayer->SetVolume(volume);
}

float AudioPlayerController::GetVolume()
{
    return m_AudioPlayer->GetVolume();
}

std::vector<AudioMeta> AudioPlayerController::GetAvaliableAudios()
{
    std::lock_guard lock(m_Mutex);
    return m_AudioStorage.List();
}

void AudioPlayerController::OnPlaybackFinished()
{
    std::lock_guard lock(m_Mutex);
    std::optional<Playback> next_pb = GetNextPlayback();
    if(!next_pb.has_value()) return;
    m_Queue.Remove(next_pb->Id);
    StartPlayback(*next_pb);
}



std::optional<Playback> AudioPlayerController::GetNextPlayback()
{
    std::vector<Playback> playbacks = m_Queue.Get();

    if(playbacks.size() == 0) return std::nullopt;

    std::optional<Playback> selected = std::nullopt;
    for(auto& pb : playbacks)
    {
        if(!m_AudioStorage.Exists(pb.AudioName))
        {
            m_Queue.Remove(pb.Id);
        }
        else if(!selected.has_value() || pb.Priority > selected->Priority || (pb.Priority == selected->Priority && pb.Time < selected->Time))
        {
            selected = pb;
        }
    }

    return selected;
}

void AudioPlayerController::StartPlayback(const Playback& playback)
{
    m_CurrentPlayback = playback;
    Audio audio = m_AudioStorage.Get(playback.AudioName);
    m_AudioPlayer->Play(audio);
}
