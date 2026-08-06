#ifndef INCLUDE_INFRASTRUCTURE_FSAUDIOSTORAGE_H_
#define INCLUDE_INFRASTRUCTURE_FSAUDIOSTORAGE_H_

#include "core/IAudioStorage.h"
#include "core/IAudioMetaReader.h"
#include <filesystem>
#include <mutex>

class FsAudioStorage : public IAudioStorage
{
public:
    FsAudioStorage(const std::filesystem::path& path, IAudioMetaReader& meta_reader);

    void Add(Audio& audio) override;
    void Remove(std::string name) override;
    bool Exists(std::string name) override;
    Audio Get(std::string name) override;
    std::vector<AudioMeta> List() override;

private:
    std::mutex m_Mutex;

    std::filesystem::path m_Path;
    IAudioMetaReader& m_MetaReader;

    std::vector<std::byte> ReadFile(const std::filesystem::path& path);
};

#endif // INCLUDE_INFRASTRUCTURE_FSAUDIOSTORAGE_H_
