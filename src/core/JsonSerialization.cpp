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


void to_json(nlohmann::json& j, const Playback& pb)
{
    j = {
        { "id", pb.Id },
        { "audioName", pb.AudioName },
        { "priority", pb.Priority },
        { "time", pb.Time }
    };
}

// void from_json(const nlohmann::json& j, Playback& pb)
// {
//     j.at("id").get_to(pb.Id);
//     j.at("audioName").get_to(pb.AudioName);
//     j.at("priority").get_to(pb.Priority);
//     j.at("time").get_to(pb.Time);
// }
