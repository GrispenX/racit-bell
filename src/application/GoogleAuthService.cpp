#include "application/GoogleAuthService.h"
#include "httplib.h"
#include "jwt.h"
#include "traits/nlohmann-json/defaults.h"

GoogleAuthService::GoogleAuthService(const std::string& client_id) :
    m_ClientId(client_id)
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

void GoogleAuthService::AddEmail(const std::string& email)
{
    m_EmailWhitelist.insert(email);
}



std::optional<std::string> GoogleAuthService::GetKey(const std::string& kid)
{
    if(std::chrono::system_clock::now() < m_KeysExpiresAt)
    {
        FetchKeys();
    }

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
    m_Keys = nlohmann::json::parse(response->body);

    std::string cache_control = response->get_header_value("Cache-Control");
    const std::string prefix = "max-age=";
    const auto pos = cache_control.find(prefix);
    const auto begin = pos + prefix.size();
    const auto end = cache_control.find(',', begin);
    const std::string value = cache_control.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
    const long long seconds = std::stoll(value);
    m_KeysExpiresAt = std::chrono::system_clock::now() + std::chrono::seconds(seconds);
}
