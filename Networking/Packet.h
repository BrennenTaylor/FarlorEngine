#pragma once

namespace Farlor
{
    struct Packet
    {
        char* m_data;
        int m_dataLength;

        Packet();
        Packet(int dataLength);
        Packet(const char* pData, int dataLength);
    };
}
