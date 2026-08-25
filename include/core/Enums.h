#ifndef INCLUDE_CORE_ENUMS_H_
#define INCLUDE_CORE_ENUMS_H_

enum class AudioPlayerControllerErrorCode
{
    INVALID_AUDIO_NAME,
    INVALID_ID,
    NOTHING_PLAYING
};

enum class DeferredPlaybackServiceErrorCode
{
    INVALID_AUDIO_NAME,
    INVALID_ID
};

enum class ScheduleServiceErrorCode
{
    SCHEDULE_NAME_ALREADY_TAKEN,
    SCHEDULE_EMPTY,
    INVALID_SCHEDULE_NAME,
    INVALID_AUDIO_NAME
};

#endif // INCLUDE_CORE_ENUMS_H_
