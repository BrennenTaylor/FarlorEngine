#include "StringUtil.h"

#include <string>

namespace Farlor
{
    std::wstring StringToWideString(const std::string& str)
    {
        std::wstring convertedWString(str.begin(), str.end());
        return convertedWString;
    }

    std::string WideStringToString(const std::wstring& wstr)
    {
        std::string convertedStr(wstr.begin(), wstr.end());
        return convertedStr;
    }
}
