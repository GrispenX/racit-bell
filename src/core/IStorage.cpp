#include "core/IStorage.h"

StorageException::StorageException(const std::string& msg) :
    m_Message(msg)
{}

const char* StorageException::what() const noexcept
{
    return m_Message.c_str();
}
