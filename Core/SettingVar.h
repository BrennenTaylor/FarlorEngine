#pragma once

#include <string>

namespace Farlor {
class SystemVar {
   public:
    SystemVar();

   private:
    std::string m_name = "DefaultName";
    std::string m_description = "Default Description";

    SystemVar *m_pNextSystemVar = nullptr;
};

class SystemVarSystem {
   public:
    SystemVarSystem();

   private:
    SystemVar *m_pSystemVar;
};

}