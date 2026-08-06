#ifndef INCLUDE_CORE_JSONSERIALIZATION_H_
#define INCLUDE_CORE_JSONSERIALIZATION_H_

#include "core/Audio.h"
#include "core/Playback.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <format>
#include <sstream>
#include <stdexcept>

namespace nlohmann
{
    template <>
    struct adl_serializer<std::chrono::system_clock::time_point>
    {
        static void to_json(json& j, const std::chrono::system_clock::time_point& tp)
        {
            j = std::format("{:%FT%T}", std::chrono::floor<std::chrono::seconds>(tp));
        }

        static void from_json(const json& j, std::chrono::system_clock::time_point& tp)
        {
            std::string s = j.get<std::string>();

            int y, mon, d, h, min, sec;
            char dash1, dash2, t, colon1, colon2;

            std::istringstream iss(s);

            if (!(iss >> y >> dash1 >> mon >> dash2 >> d
                    >> t
                    >> h >> colon1 >> min >> colon2 >> sec) ||
                dash1 != '-' || dash2 != '-' ||
                t != 'T' ||
                colon1 != ':' || colon2 != ':')
            {
                throw std::invalid_argument("Invalid datetime format");
            }

            std::chrono::year_month_day ymd{
                std::chrono::year{y},
                std::chrono::month{static_cast<unsigned>(mon)},
                std::chrono::day{static_cast<unsigned>(d)}
            };

            if (!ymd.ok())
                throw std::invalid_argument("Invalid date");

            tp = std::chrono::sys_days{ymd}
            + std::chrono::hours{h}
            + std::chrono::minutes{min}
            + std::chrono::seconds{sec};
        }
    };
}


void to_json(nlohmann::json& j, const AudioMeta& meta);
// void from_json(const nlohmann::json& j, AudioMeta& meta);

void to_json(nlohmann::json& j, const Playback& pb);
// void from_json(const nlohmann::json& j, Playback& pb);

#endif // INCLUDE_CORE_JSONSERIALIZATION_H_
