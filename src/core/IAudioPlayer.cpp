#include "core/IAudioPlayer.h"

AudioPlayerException::AudioPlayerException(const std::string& msg) :
    m_Message(msg)
{}

const char* AudioPlayerException::what() const noexcept
{
    return m_Message.c_str();
}
