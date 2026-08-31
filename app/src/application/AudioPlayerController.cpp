#include "application/AudioPlayerController.h"
#include <algorithm>
#include <format>

AudioPlayerControllerException::AudioPlayerControllerException(AudioPlayerControllerErrorCode code, const std::string& msg) :
    m_Code(code),
    m_Message(msg)
{}

const char* AudioPlayerControllerException::what() const noexcept
{
    return m_Message.c_str();
}

AudioPlayerControllerErrorCode AudioPlayerControllerException::code() const noexcept
{
    return m_Code;
}


AudioPlayerController::AudioPlayerController(std::unique_ptr<IAudioPlayer> audio_player, IAudioStorage& audio_storage, IStorage<int, QueuedPlayback>& queued_pb_storage) :
    m_AudioPlayer(std::move(audio_player)),
    m_AudioStorage(audio_storage),
    m_Queue(queued_pb_storage)
{
    m_AudioPlayer->Subscribe(this);
}


void AudioPlayerController::Enqueue(const QueuedPlayback& pb)
{
    std::lock_guard lock(m_Mutex);
    
    if(!m_AudioStorage.Exists(pb.AudioName))
    {
        throw AudioPlayerControllerException(
            AudioPlayerControllerErrorCode::INVALID_AUDIO_NAME,
            std::format("Audio with name '{}' does not exist.", pb.AudioName)
        );
    }

    if(!m_AudioPlayer->IsPlaying())
    {
        StartPlayback(pb);
    }
    else if(m_CurrentPb.Priority < pb.Priority)
    {
        m_AudioPlayer->Stop();
        StartPlayback(pb);
    }
    else
    {
        QueuedPlayback to_add = pb;
        to_add.EnqueuedAt = std::chrono::system_clock::now();
        m_Queue.Add(to_add);
    }
}

void AudioPlayerController::Dequeue(int id)
{
    std::lock_guard lock(m_Mutex);
    if(!m_Queue.Exists(id))
    {
        throw AudioPlayerControllerException(
            AudioPlayerControllerErrorCode::INVALID_ID,
            std::format("Queued playback with id '{}' does not exist.", id)
        );
    }
    m_Queue.Remove(id);
}

std::vector<QueuedPlayback> AudioPlayerController::GetQueuedPlaybacks()
{
    std::lock_guard lock(m_Mutex);
    return m_Queue.Get();
}

void AudioPlayerController::StopCurrent()
{
    std::lock_guard lock(m_Mutex);
    if(!m_AudioPlayer->IsPlaying())
    {
        throw AudioPlayerControllerException(
            AudioPlayerControllerErrorCode::NOTHING_PLAYING,
            "Nothing is playing right now."
        );
    }
    m_AudioPlayer->Stop();
    std::optional<QueuedPlayback> next_pb = GetNextPlayback();
    if(!next_pb.has_value()) return;
    m_Queue.Remove(next_pb->Id);
    StartPlayback(*next_pb);
}

void AudioPlayerController::SetVolume(float volume)
{
    std::lock_guard lock(m_Mutex);
    m_AudioPlayer->SetVolume(volume);
}

float AudioPlayerController::GetVolume()
{
    std::lock_guard lock(m_Mutex);
    return m_AudioPlayer->GetVolume();
}

void AudioPlayerController::OnPlaybackFinished()
{
    std::lock_guard lock(m_Mutex);
    std::optional<QueuedPlayback> next_pb = GetNextPlayback();
    if(!next_pb.has_value()) return;
    m_Queue.Remove(next_pb->Id);
    StartPlayback(*next_pb);
}



std::optional<QueuedPlayback> AudioPlayerController::GetNextPlayback()
{
    std::vector<QueuedPlayback> playbacks = m_Queue.Get();

    if(playbacks.empty()) return std::nullopt;

    std::optional<QueuedPlayback> selected = std::nullopt;
    for(auto& pb : playbacks)
    {
        if(!m_AudioStorage.Exists(pb.AudioName))
        {
            m_Queue.Remove(pb.Id);
        }
        else if(!selected.has_value() || pb.Priority > selected->Priority || (pb.Priority == selected->Priority && pb.EnqueuedAt < selected->EnqueuedAt))
        {
            selected = pb;
        }
    }

    return selected;
}

void AudioPlayerController::StartPlayback(const QueuedPlayback& pb)
{
    m_CurrentPb = pb;
    Audio audio = m_AudioStorage.Get(pb.AudioName);
    m_AudioPlayer->Play(audio);
}
