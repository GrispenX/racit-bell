#include "infrastructure/FsAudioStorage.h"
#include <stdexcept>
#include <format>
#include <fstream>

FsAudioStorage::FsAudioStorage(const std::filesystem::path& path, IAudioMetaReader& meta_reader) :
    m_Path(path),
    m_MetaReader(meta_reader)
{

}

void FsAudioStorage::Add(Audio& audio)
{
    std::lock_guard lock(m_Mutex);
    std::filesystem::path path = m_Path / audio.Meta.Name;
    if(std::filesystem::exists(path))
        throw std::runtime_error(std::format("Audio with name {} already exists.", audio.Meta.Name));
    std::ofstream ofile(path, std::ios::binary);
    ofile.write(reinterpret_cast<const char*>(audio.Data.data()), audio.Data.size());
}

void FsAudioStorage::Remove(std::string name)
{
    std::lock_guard lock(m_Mutex);
    std::filesystem::remove(m_Path / name);
}

bool FsAudioStorage::Exists(std::string name)
{
    std::lock_guard lock(m_Mutex);
    return std::filesystem::exists(m_Path / name);
}

Audio FsAudioStorage::Get(std::string name)
{
    std::lock_guard lock(m_Mutex);
    std::filesystem::path path = m_Path / name;
    if(!std::filesystem::exists(path))
        throw std::runtime_error(std::format("Audio with name {} does not exist", name));
    
    Audio audio;
    audio.Data = ReadFile(path);
    audio.Meta = m_MetaReader.ReadMeta(audio.Data);
    audio.Meta.Name = name;
    return audio;
}

std::vector<AudioMeta> FsAudioStorage::List()
{
    std::lock_guard lock(m_Mutex);
    std::vector<AudioMeta> result;
    for(const auto& entry : std::filesystem::directory_iterator(m_Path))
    {
        if(!entry.is_regular_file()) continue;
        std::vector<std::byte> data = ReadFile(entry.path());
        AudioMeta meta = m_MetaReader.ReadMeta(data);
        meta.Name = entry.path().filename().string();
        result.push_back(meta);
    }
    return result;
}

std::vector<std::byte> FsAudioStorage::ReadFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0);
    std::vector<std::byte> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();
    return data;
}