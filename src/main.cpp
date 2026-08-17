#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#include "core/JsonSerialization.h"
#include "infrastructure/MiniaudioPlayer.h"
#include "infrastructure/FsAudioStorage.h"
#include "application/AudioPlayerController.h"
#include "application/MiniaudioMetaReader.h"
#include "application/PlaybackScheduler.h"
#include "application/GoogleAuthService.h"

#include "httplib.h"

#include <iostream>

int main()
{
    const std::string google_client_id = "191974541456-4qb5q4apnds2f7qf6jnealr94irafaae.apps.googleusercontent.com";

    MiniaudioMetaReader meta_reader;
    FsAudioStorage audio_storage("../sounds", meta_reader);
    AudioPlayerController player_controller(std::make_unique<MiniaudioPlayer>(), audio_storage);
    PlaybackScheduler scheduler(player_controller, audio_storage);

    GoogleAuthService google_auth(google_client_id);
    google_auth.AddEmail("kovalchuk.iu_ipz23@rcit.ukr.education");

    httplib::Server server;

    server.set_pre_routing_handler([&](const httplib::Request& req, httplib::Response& res) {
        const std::string auth = req.get_header_value("Authorization");
        const std::string prefix = "Bearer ";

        if(auth.empty() || !auth.starts_with(prefix))
        {
            std::cout << "No token\n";
            res.status = 401;
            return httplib::Server::HandlerResponse::Handled;
        }

        const std::string token = auth.substr(prefix.size());

        if(token.empty() || !google_auth.VerifyIdToken(token))
        {
            std::cout << "Token not verified\n";
            res.status = 401;
            return httplib::Server::HandlerResponse::Handled;
        }

        return httplib::Server::HandlerResponse::Unhandled;
    });

    server.Get("/sounds", [&](const httplib::Request& req, httplib::Response& res) {
        std::vector<AudioMeta> sounds = player_controller.GetAvaliableAudios();
        nlohmann::json j = sounds;
        res.set_content(j.dump(), "application/json");
    });

    server.Get(R"(/sounds/([A-Za-z0-9_.-]+))", [&](const httplib::Request& req, httplib::Response& res) {
        Audio audio;
        try
        {
            audio = audio_storage.Get(req.matches[1]);
        }
        catch(const std::exception& e)
        {
            res.status = 404;
            res.set_content("Audio not found.", "text/plain");
            return;
        }
        res.set_content(reinterpret_cast<const char*>(audio.Data.data()), audio.Data.size(), "application/octet-stream");
    });

    server.Post("/sounds", [&](const httplib::Request& req, httplib::Response& res) {
        if(!req.form.has_file("file"))
        {
            res.set_content("Expected a file", "text/plain");
            res.status = 400;
            return;
        }
        const auto& file = req.form.get_file("file");
        std::vector<std::byte> audio_data(file.content.size());
        memcpy(audio_data.data(), file.content.data(), file.content.size());
        Audio audio = {
            .Meta = {
                .Name = file.filename
            },
            .Data = audio_data
        };
        audio_storage.Add(audio);
    });

    server.Delete(R"(/sounds/([A-Za-z0-9_.-]+))", [&](const httplib::Request& req, httplib::Response& res) {
        if(!audio_storage.Exists(req.matches[1]))
        {
            res.status = 404;
            res.set_content("Audio not found.", "text/plain");
        }
        else
        {
            audio_storage.Remove(req.matches[1]);
        }
    });

    server.Get("/volume", [&](const httplib::Request& req, httplib::Response& res) {
        float volume = player_controller.GetVolume();
        nlohmann::json j = {{"volume", volume}};
        res.set_content(j.dump(), "application/json");
    });

    server.Post("/volume", [&](const httplib::Request& req, httplib::Response& res) {
        float volume;
        try
        {
            nlohmann::json body = nlohmann::json::parse(req.body);
            volume = body["volume"].get<float>();
        }
        catch(...)
        {
            res.set_content("Invalid request body.", "text/plain");
            res.status = 400;
            return;
        }

        try
        {
            player_controller.SetVolume(volume);
        }
        catch(const std::exception& e)
        {
            res.set_content(e.what(), "text/plain");
            res.status = 400;
        }
    });

    server.Get("/queue", [&](const httplib::Request& req, httplib::Response& res) {
        std::vector<Playback> queue = player_controller.GetQueuedPlaybacks();
        nlohmann::json j = queue;
        res.set_content(j.dump(), "application/json");
    });

    server.Post("/queue", [&](const httplib::Request& req, httplib::Response& res) {
        std::string audio_name;
        int priority;
        try
        {
            nlohmann::json body = nlohmann::json::parse(req.body);
            audio_name = body["audioName"].get<std::string>();
            priority = body["priority"].get<int>();
        }
        catch(...)
        {
            res.set_content("Invalid request body.", "text/plain");
            res.status = 400;
            return;
        }

        try
        {
            player_controller.Enqueue({.AudioName = audio_name, .Priority = priority});
        }
        catch(const std::exception& e)
        {
            res.set_content(e.what(), "text/plain");
            res.status = 400;
        }
        
    });

    server.Delete(R"(/queue/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        player_controller.Dequeue(id);
    });

    server.Post("/queue/skip", [&](const httplib::Request& req, httplib::Response& res) {
        player_controller.StopCurrent();
    });

    server.Get("/schedule", [&](const httplib::Request& req, httplib::Response& res) {
        std::vector<Playback> schedule = scheduler.GetScheduledPlaybacks();
        nlohmann::json j = schedule;
        res.set_content(j.dump(), "application/json");
    });

    server.Post("/schedule", [&](const httplib::Request& req, httplib::Response& res) {
        std::string audio_name;
        int priority;
        std::chrono::system_clock::time_point time;
        try
        {
            nlohmann::json body = nlohmann::json::parse(req.body);
            audio_name = body["audioName"].get<std::string>();
            priority = body["priority"].get<int>();
            time = body["time"].get<std::chrono::system_clock::time_point>();
        }
        catch(...)
        {
            res.set_content("Invalid request body.", "text/plain");
            res.status = 400;
            return;
        }

        try
        {   
            scheduler.Schedule({.AudioName = audio_name, .Priority = priority, .Time = time});
        }
        catch(const std::exception& e)
        {
            res.set_content(e.what(), "text/plain");
            res.status = 400;
        }
    });

    server.Delete(R"(/schedule/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        scheduler.Unschedule(id);
    });

    server.listen("0.0.0.0", 8080);
}