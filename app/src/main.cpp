#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "core/JsonSerialization.h"
#include "infrastructure/MiniaudioPlayer.h"
#include "infrastructure/FsAudioStorage.h"
#include "infrastructure/InMemStorage.h"
#include "infrastructure/JsonScheduleStorage.h"
#include "application/MiniaudioMetaReader.h"
#include "application/AudioPlayerController.h"
#include "application/DeferredPlaybackService.h"
#include "application/ScheduleService.h"
#include "application/GoogleAuthService.h"

#include "httplib.h"

int main()
{
    const std::filesystem::path base_path("server-data");
    std::ifstream file(base_path / "config.json");
    const nlohmann::json config = nlohmann::json::parse(file);
    const std::string google_client_id = config["google_client_id"];
    const std::set<std::string> allowed_emails = config["allowed_emails"];
    const bool use_auth = config["use_auth"];
    const int port = config["port"];

    MiniaudioMetaReader meta_reader;
    FsAudioStorage audio_storage(base_path / "sounds", meta_reader);

    InMemStorage<QueuedPlayback> q_pb_storage;
    InMemStorage<DeferredPlayback> d_pb_storage;
    JsonScheduleStorage schedule_storage(base_path / "schedules.json");

    AudioPlayerController player_controller(std::make_unique<MiniaudioPlayer>(), audio_storage, q_pb_storage);
    DeferredPlaybackService deferred_pb_serv(d_pb_storage, player_controller, audio_storage);
    ScheduleService schedule_serv(schedule_storage, player_controller, audio_storage);

    GoogleAuthService google_auth(google_client_id, allowed_emails);

    httplib::Server server;

    if(use_auth)
    {
        server.set_pre_routing_handler([&](const httplib::Request& req, httplib::Response& res) {
            const std::string auth = req.get_header_value("Authorization");
            const std::string prefix = "Bearer ";
    
            if(auth.empty() || !auth.starts_with(prefix))
            {
                res.status = 401;
                return httplib::Server::HandlerResponse::Handled;
            }
    
            const std::string token = auth.substr(prefix.size());
    
            if(token.empty() || !google_auth.VerifyIdToken(token))
            {
                res.status = 401;
                return httplib::Server::HandlerResponse::Handled;
            }
    
            return httplib::Server::HandlerResponse::Unhandled;
        });
    }

    server.set_exception_handler([&](const httplib::Request& req, httplib::Response& res, std::exception_ptr ep) {
        try
        {
            std::rethrow_exception(ep);
        }
        catch(const nlohmann::json::exception& e)
        {
            nlohmann::json j = {
                { "error", "INVALID_BODY" },
                { "message", "Invalid request body."}
            };
            res.status = 400;
            res.set_content(j.dump(), "application/json");
        }
        catch(const AudioPlayerControllerException& e)
        {
            nlohmann::json j = {
                { "error", e.code() },
                { "message", e.what() }
            };
            res.status = 400;
            res.set_content(j.dump(), "application/json");
        }
        catch(const DeferredPlaybackServiceException& e)
        {
            nlohmann::json j = {
                { "error", e.code() },
                { "message", e.what() }
            };
            res.status = 400;
            res.set_content(j.dump(), "application/json");
        }
        catch(const ScheduleServiceException& e)
        {
            nlohmann::json j = {
                { "error", e.code() },
                { "message", e.what() }
            };
            res.status = 400;
            res.set_content(j.dump(), "application/json");
        }
        catch(const std::exception& e)
        {
            res.status = 500;
            res.set_content("Internal server error", "text/plain");
            std::cerr << e.what() << "\n";
        }
        
    });

    server.Get("/sounds", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j = audio_storage.List();;
        res.set_content(j.dump(), "application/json");
    });

    server.Get(R"(/sounds/([^/]+))", [&](const httplib::Request& req, httplib::Response& res) {
        Audio audio;
        try
        {
            audio = audio_storage.Get(req.matches[1]);
            res.set_content(reinterpret_cast<const char*>(audio.Data.data()), audio.Data.size(), "application/octet-stream");
        }
        catch(const std::runtime_error& e)
        {
            res.status = 404;
            res.set_content(e.what(), "text/plain");
        }
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
        
        ma_decoder decoder;
        if(ma_decoder_init_memory(audio_data.data(), audio_data.size(), nullptr, &decoder) != MA_SUCCESS)
        {
            res.status = 400;
            res.set_content("Bad or unsupported audio.", "text/plain");
        }
        ma_decoder_uninit(&decoder);

        Audio audio = {
            .Meta = {
                .Name = file.filename
            },
            .Data = audio_data
        };
        
        audio_storage.Add(audio);
    });

    server.Delete(R"(/sounds/([^/]+))", [&](const httplib::Request& req, httplib::Response& res) {
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
        nlohmann::json body = nlohmann::json::parse(req.body);
        float volume = body["volume"].get<float>();
        player_controller.SetVolume(volume);
    });

    server.Get("/queue", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j = player_controller.GetQueuedPlaybacks();
        res.set_content(j.dump(), "application/json");
    });

    server.Post("/queue", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body = nlohmann::json::parse(req.body);
        QueuedPlayback pb = body;
        player_controller.Enqueue(pb);     
    });

    server.Delete(R"(/queue/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        player_controller.Dequeue(id);
    });

    server.Post("/queue/skip", [&](const httplib::Request& req, httplib::Response& res) {
        player_controller.StopCurrent();
    });

    server.Get("/deferred", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j = deferred_pb_serv.GetDeferredPlaybacks();
        res.set_content(j.dump(), "application/json");
    });

    server.Post("/deferred", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body = nlohmann::json::parse(req.body);
        DeferredPlayback pb = body;
        deferred_pb_serv.Schedule(pb);
    });

    server.Delete(R"(/deferred/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        deferred_pb_serv.Unschedule(id);
    });

    server.Get("/schedule", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json j = schedule_storage.Get();
        res.set_content(j.dump(), "application/json");
    });

    server.Post("/schedule", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body = nlohmann::json::parse(req.body);
        Schedule schedule = body;
        schedule_serv.AddSchedule(schedule);
    });

    server.Put("/schedule", [&](const httplib::Request& req, httplib::Response& res) {
        nlohmann::json body = nlohmann::json::parse(req.body);
        Schedule schedule = body;
        schedule_serv.UpdateSchedule(schedule);
    });

    server.Post(R"(/schedule/([^/]+)/state)", [&](const httplib::Request& req, httplib::Response& res) {
        std::string name = req.matches[1].str();
        nlohmann::json body = nlohmann::json::parse(req.body);
        bool state = body["state"].get<bool>();
        schedule_serv.SetScheduleState(name, state);
    });

    server.Delete(R"(/schedule/([^/]+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string name = req.matches[1].str();
        schedule_serv.RemoveSchedule(name);
    });

    server.listen("0.0.0.0", port);
}