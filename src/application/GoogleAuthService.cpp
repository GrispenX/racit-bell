#include "application/GoogleAuthService.h"
#include "httplib.h"
#include "jwt.h"
#include "traits/nlohmann-json/defaults.h"

GoogleAuthService::GoogleAuthService(const std::string& client_id, const std::set<std::string>& emails) :
    m_ClientId(client_id),
    m_EmailWhitelist(emails)
{
    FetchKeys();
}

bool GoogleAuthService::VerifyIdToken(const std::string& token)
{
    try
    {
        auto decoded = jwt::decode(token);
        
        const std::string kid = decoded.get_key_id();
        std::optional<std::string> key = GetKey(kid);
        if(!key.has_value())
        {
            return false;
        }
    
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::rs256(key.value()))
            .with_issuer("https://accounts.google.com")
            .with_audience(m_ClientId);
    
        verifier.verify(decoded);
    
        nlohmann::json payload = nlohmann::json::parse(decoded.get_payload());
        if(!payload.contains("email") || !payload["email"].is_string())
        {
            return false;
        }
        const std::string email = payload["email"].get<std::string>();
        if(!m_EmailWhitelist.contains(email))
        {
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        return false;
    }
}


std::optional<std::string> GoogleAuthService::GetKey(const std::string& kid)
{
    bool keys_expired;
    {
        std::lock_guard lock(m_KeysMutex);
        keys_expired = std::chrono::system_clock::now() > m_KeysExpiresAt;
    }

    if(keys_expired)
    {
        FetchKeys();
    }

    std::lock_guard lock(m_KeysMutex);
    if(!m_Keys.contains(kid) || !m_Keys[kid].is_string())
    {
        return std::nullopt;
    }

    return m_Keys[kid].get<std::string>();
}

void GoogleAuthService::FetchKeys()
{
    httplib::SSLClient client("www.googleapis.com");
    auto response = client.Get("/oauth2/v1/certs");
    if(!response || response->status != 200)
    {
        return;
    }
    nlohmann::json keys = nlohmann::json::parse(response->body);

    std::string cache_control = response->get_header_value("Cache-Control");
    const std::string prefix = "max-age=";
    const auto pos = cache_control.find(prefix);
    const auto begin = pos + prefix.size();
    const auto end = cache_control.find(',', begin);
    const std::string value = cache_control.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
    const long long seconds = std::stoll(value);
    const auto expires_at = std::chrono::system_clock::now() + std::chrono::seconds(seconds);

    {
        std::lock_guard lock(m_KeysMutex);
        m_Keys = std::move(keys);
        m_KeysExpiresAt = expires_at;
    }
}
