#include "HashedString.h"

namespace Farlor
{
    HashedString::HashedString()
        : m_id{HashString("")}
        , m_hashedString{""}
    {
    }

    HashedString::HashedString(std::string stringToHash)
        : m_id{HashString(stringToHash)}
        , m_hashedString{stringToHash}
    {
    }

    uint64_t HashedString::HashString(const std::string stringToHash)
    {
        return std::hash<std::string>()(stringToHash);
    }
}
