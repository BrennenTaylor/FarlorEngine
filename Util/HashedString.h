#pragma once

#include <cstdint>
#include <string>

namespace Farlor
{
    class HashedString
    {
    public:
        HashedString();
        HashedString(std::string stringToHash);

        static uint64_t HashString(std::string stringToHash);

        bool operator<(const HashedString& other) const
        {
            return (m_id < other.m_id);
        }
        bool operator==(const HashedString& other) const
        {
            return (m_id == other.m_id);
        }

        // Store as void* so it is displayed as hex for debugger
        uint64_t m_id;
        std::string m_hashedString;
    };
}
