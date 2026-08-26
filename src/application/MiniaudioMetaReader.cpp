#include "application/MiniaudioMetaReader.h"
#include <miniaudio.h>
#include <stdexcept>

AudioMeta MiniaudioMetaReader::ReadMeta(std::span<const std::byte> audio)
{
    ma_decoder decoder;

    ma_result result = ma_decoder_init_memory(audio.data(), audio.size(), nullptr, &decoder);
    if(result != MA_SUCCESS) throw std::runtime_error("Failed to decode audio");

    ma_uint64 frame_count = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count);

    AudioMeta meta;
    meta.Duration = std::chrono::milliseconds(frame_count * 1000 / decoder.outputSampleRate);

    ma_decoder_uninit(&decoder);
    return meta;
}
