#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#include "core/JsonSerialization.h"
#include "infrastructure/MiniaudioPlayer.h"
#include "infrastructure/FsAudioStorage.h"
#include "infrastructure/InMemStorage.h"
#include "application/MiniaudioMetaReader.h"
#include "application/AudioPlayerController.h"
#include "application/DeferredPlaybackService.h"
#include "application/ScheduleService.h"
#include "application/GoogleAuthService.h"

#include "httplib.h"

class InMemScheduleStorage : public IStorage<std::string, Schedule>
{
public:
    std::string Add(const Schedule& value) override
    {
        std::lock_guard lock(m_Mutex);
        if(m_Data.find(value.Name) != m_Data.end())
        {
            throw StorageException(std::format("Schedule with name '{}' already exists.", value.Name));
        }
        m_Data.insert({value.Name, value});
        return value.Name;
    }

    void Update(const Schedule& value) override
    {
        std::lock_guard lock(m_Mutex);
        auto it = m_Data.find(value.Name);
        if(it == m_Data.end())
        {
            throw StorageException(std::format("Schedule with id '{}' does not exist.", value.Name));
        }
        it->second = value;
    }

    void Remove(const std::string& name) override
    {
        std::lock_guard lock(m_Mutex);
        if(m_Data.find(name) == m_Data.end())
        {
            throw StorageException(std::format("Schedule with id '{}' does not exist.", name));
        }
        m_Data.erase(name);
    }

    bool Exists(const std::string& name) override
    {
        std::lock_guard lock(m_Mutex);
        return m_Data.find(name) != m_Data.end();
    }

    Schedule Get(const std::string& name) override
    {
        std::lock_guard lock(m_Mutex);
        auto it = m_Data.find(name);
        if(it == m_Data.end())
        {
            throw StorageException(std::format("Schedule with id '{}' does not exist.", name));
        }
        return it->second;
    }

    std::vector<Schedule> Get(std::function<bool(const Schedule&)> predicate) override
    {
        std::lock_guard lock(m_Mutex);
        std::vector<Schedule> suitable;
        for(auto& [_, value] : m_Data)
        {
            if(predicate(value)) suitable.push_back(value);
        }
        return suitable;
    }

    std::vector<Schedule> Get() override
    {
        std::lock_guard lock(m_Mutex);
        std::vector<Schedule> values;
        for(auto& [_, value] : m_Data)
        {
            values.push_back(value);
        }
        return values;
    }

private:
    std::unordered_map<std::string, Schedule> m_Data;
    std::mutex m_Mutex;
};

int main()
{
    // const std::string google_client_id = "191974541456-4qb5q4apnds2f7qf6jnealr94irafaae.apps.googleusercontent.com";

    MiniaudioMetaReader meta_reader;
    FsAudioStorage audio_storage("../sounds", meta_reader);

    InMemStorage<QueuedPlayback> q_pb_storage;
    InMemStorage<DeferredPlayback> d_pb_storage;
    InMemScheduleStorage schedule_storage;

    AudioPlayerController player_controller(std::make_unique<MiniaudioPlayer>(), audio_storage, q_pb_storage);
    DeferredPlaybackService deferred_pb_serv(d_pb_storage, player_controller, audio_storage);
    ScheduleService schedule_serv(schedule_storage, player_controller, audio_storage);

    // GoogleAuthService google_auth(google_client_id);
    // google_auth.AddEmail("kovalchuk.iu_ipz23@rcit.ukr.education");

    httplib::Server server;

    // server.set_pre_routing_handler([&](const httplib::Request& req, httplib::Response& res) {
    //     const std::string auth = req.get_header_value("Authorization");
    //     const std::string prefix = "Bearer ";

    //     if(auth.empty() || !auth.starts_with(prefix))
    //     {
    //         std::cout << "No token\n";
    //         res.status = 401;
    //         return httplib::Server::HandlerResponse::Handled;
    //     }

    //     const std::string token = auth.substr(prefix.size());

    //     if(token.empty() || !google_auth.VerifyIdToken(token))
    //     {
    //         std::cout << "Token not verified\n";
    //         res.status = 401;
    //         return httplib::Server::HandlerResponse::Handled;
    //     }

    //     return httplib::Server::HandlerResponse::Unhandled;
    // });

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
        catch(const StorageException& e)
        {
            res.status = 404;
            res.set_content("Audio not found.", "text/plain");
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

    server.listen("0.0.0.0", 8080);
}