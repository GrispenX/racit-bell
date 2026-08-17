#ifndef INCLUDE_APPLICATION_GOOGLEAUTHSERVICE_H_
#define INCLUDE_APPLICATION_GOOGLEAUTHSERVICE_H_

#include <string>
#include <chrono>
#include <optional>
#include <set>
#include <nlohmann/json.hpp>

class GoogleAuthService
{
public:
    GoogleAuthService(const std::string& client_id);

    // Returns email if verified
    bool VerifyIdToken(const std::string& token);
    void AddEmail(const std::string& email);

private:
    std::string m_ClientId;

    nlohmann::json m_Keys;
    std::chrono::system_clock::time_point m_KeysExpiresAt;
    std::optional<std::string> GetKey(const std::string& kid);
    void FetchKeys();

    std::set<std::string> m_EmailWhitelist;
};

#endif // INCLUDE_APPLICATION_GOOGLEAUTHSERVICE_H_
