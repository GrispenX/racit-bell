#ifndef INCLUDE_CORE_JSONSERIALIZATION_H_
#define INCLUDE_CORE_JSONSERIALIZATION_H_

#include "core/Audio.h"
#include "core/Enums.h"
#include "core/QueuedPlayback.h"
#include "core/DeferredPlayback.h"
#include "core/Schedule.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <format>
#include <sstream>
#include <stdexcept>

NLOHMANN_JSON_SERIALIZE_ENUM(AudioPlayerControllerErrorCode, {
    { AudioPlayerControllerErrorCode::INVALID_AUDIO_NAME, "INVALID_AUDIO_NAME" },
    { AudioPlayerControllerErrorCode::INVALID_ID, "INVALID_ID" },
    { AudioPlayerControllerErrorCode::NOTHING_PLAYING, "NOTHING_PLAYING" }
})

NLOHMANN_JSON_SERIALIZE_ENUM(DeferredPlaybackServiceErrorCode, {
    { DeferredPlaybackServiceErrorCode::INVALID_AUDIO_NAME, "INVALID_AUDIO_NAME" },
    { DeferredPlaybackServiceErrorCode::INVALID_ID, "INVALID_ID" }
})

NLOHMANN_JSON_SERIALIZE_ENUM(ScheduleServiceErrorCode, {
    { ScheduleServiceErrorCode::INVALID_SCHEDULE_NAME, "INVALID_SCHEDULE_NAME" },
    { ScheduleServiceErrorCode::SCHEDULE_EMPTY, "SCHEDULE_EMPTY" },
    { ScheduleServiceErrorCode::SCHEDULE_NAME_ALREADY_TAKEN, "SCHEDULE_NAME_ALREADY_TAKEN" },
    { ScheduleServiceErrorCode::INVALID_AUDIO_NAME, "INVALID_AUDIO_NAME" }
})

namespace nlohmann
{
    template <>
    struct adl_serializer<std::chrono::system_clock::time_point>
    {
        static void to_json(json& j, const std::chrono::system_clock::time_point& tp)
        {
            j = std::format(
                "{:%FT%TZ}",
                std::chrono::floor<std::chrono::seconds>(tp)
            );
        }

        static void from_json(
            const json& j,
            std::chrono::system_clock::time_point& tp)
        {
            std::string s = j.get<std::string>();

            int y, mon, d, h, min, sec;

            char dash1, dash2;
            char t;
            char colon1, colon2;
            char z;

            std::istringstream iss(s);

            if (!(iss >> y >> dash1 >> mon >> dash2 >> d
                    >> t
                    >> h >> colon1 >> min >> colon2 >> sec
                    >> z) ||
                dash1 != '-' ||
                dash2 != '-' ||
                t != 'T' ||
                colon1 != ':' ||
                colon2 != ':' ||
                z != 'Z')
            {
                throw json::parse_error::create(
                    101,
                    0,
                    "Invalid datetime format",
                    nullptr
                );
            }

            iss >> std::ws;

            if (!iss.eof())
            {
                throw json::parse_error::create(
                    101,
                    0,
                    "Invalid datetime format",
                    nullptr
                );
            }

            std::chrono::year_month_day ymd{
                std::chrono::year{y},
                std::chrono::month{static_cast<unsigned>(mon)},
                std::chrono::day{static_cast<unsigned>(d)}
            };

            if (!ymd.ok())
            {
                throw json::parse_error::create(
                    101,
                    0,
                    "Invalid date",
                    nullptr
                );
            }

            if (h < 0 || h > 23 ||
                min < 0 || min > 59 ||
                sec < 0 || sec > 59)
            {
                throw json::parse_error::create(
                    101,
                    0,
                    "Invalid time",
                    nullptr
                );
            }

            tp = std::chrono::sys_days{ymd}
            + std::chrono::hours{h}
            + std::chrono::minutes{min}
            + std::chrono::seconds{sec};
        }
    };

    template <>
    struct adl_serializer<std::chrono::hh_mm_ss<std::chrono::seconds>>
    {
        using Time = std::chrono::hh_mm_ss<std::chrono::seconds>;

        static void to_json(json& j, const Time& time)
        {
            j = std::format(
                "{:%H:%M:%S}",
                time
            );
        }

        static void from_json(const json& j, Time& time)
        {
            std::string s = j.get<std::string>();

            int hours, minutes, seconds;
            char colon1, colon2;

            std::istringstream iss(s);

            if (!(iss >> hours >> colon1 >> minutes >> colon2 >> seconds) ||
                colon1 != ':' ||
                colon2 != ':')
            {
                throw std::invalid_argument("Invalid time format");
            }

            if (hours < 0 || hours > 23 ||
                minutes < 0 || minutes > 59 ||
                seconds < 0 || seconds > 59)
            {
                throw std::invalid_argument("Invalid time");
            }

            time = Time{
                std::chrono::hours{hours} +
                std::chrono::minutes{minutes} +
                std::chrono::seconds{seconds}
            };
        }
    };
}


void to_json(nlohmann::json& j, const AudioMeta& meta);
// void from_json(const nlohmann::json& j, AudioMeta& meta);

void to_json(nlohmann::json& j, const QueuedPlayback& pb);
void from_json(const nlohmann::json& j, QueuedPlayback& pb);

void to_json(nlohmann::json& j, const DeferredPlayback& pb);
void from_json(const nlohmann::json& j, DeferredPlayback& pb);

void to_json(nlohmann::json& j, const ScheduleEntry& e);
void from_json(const nlohmann::json& j, ScheduleEntry& e);

void to_json(nlohmann::json& j, const Schedule& s);
void from_json(const nlohmann::json& j, Schedule& s);

#endif // INCLUDE_CORE_JSONSERIALIZATION_H_
