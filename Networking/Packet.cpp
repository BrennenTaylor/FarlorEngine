#include "Packet.h"

#include <memory>

namespace Farlor
{
    Packet::Packet()
        : m_data{nullptr}
        , m_dataLength{0}
    {
    }

    Packet::Packet(int dataLength)
    {
        m_data = new char[dataLength];
        memset(m_data, 0, dataLength);
    }

    Packet::Packet(const char* pData, int dataLength)
    {
        m_data = new char[dataLength];
        memcpy(m_data, pData, dataLength);
        m_dataLength = dataLength;
    }
}
