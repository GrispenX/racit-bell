#include "core/JsonSerialization.h"

void to_json(nlohmann::json& j, const AudioMeta& meta)
{
    j = {
        { "name", meta.Name },
        { "duration_ms", std::chrono::duration_cast<std::chrono::milliseconds>(meta.Duration).count() }
    };
}

// void from_json(const nlohmann::json& j, AudioMeta& meta)
// {
//     j.at("name").get_to(meta.Name);
//     j.at("duration_ms").get_to(meta.Duration);
// }


void to_json(nlohmann::json& j, const QueuedPlayback& pb)
{
    j = {
        { "id", pb.Id },
        { "audio_name", pb.AudioName },
        { "priority", pb.Priority },
        { "enqueued_at", pb.EnqueuedAt }
    };
}

void from_json(const nlohmann::json& j, QueuedPlayback& pb)
{
    j.at("audio_name").get_to(pb.AudioName);
    j.at("priority").get_to(pb.Priority);
}


void to_json(nlohmann::json& j, const DeferredPlayback& pb)
{
    j = {
        { "id", pb.Id },
        { "audio_name", pb.AudioName },
        { "priority", pb.Priority },
        { "play_time", pb.PlayTime }
    };
}

void from_json(const nlohmann::json& j, DeferredPlayback& pb)
{
    j.at("audio_name").get_to(pb.AudioName);
    j.at("priority").get_to(pb.Priority);
    j.at("play_time").get_to(pb.PlayTime);
}


void to_json(nlohmann::json& j, const ScheduleEntry& e)
{
    j = {
        { "audio_name", e.AudioName },
        { "priority", e.Priority },
        { "time", e.Time }
    };
}

void from_json(const nlohmann::json& j, ScheduleEntry& e)
{
    j.at("audio_name").get_to(e.AudioName);
    j.at("priority").get_to(e.Priority);
    j.at("time").get_to(e.Time);
}


void to_json(nlohmann::json& j, const Schedule& s)
{
    j = {
        { "name", s.Name },
        { "enabled", s.Enabled },
        { "entries", s.Entries }
    };
}

void from_json(const nlohmann::json& j, Schedule& s)
{
    j.at("name").get_to(s.Name);
    j.at("enabled").get_to(s.Enabled);
    j.at("entries").get_to(s.Entries);
}

