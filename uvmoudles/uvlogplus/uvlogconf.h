#pragma once
#include "uvlogprivate.h"

namespace uvLogPlus {
    Configuration* ConfigParse(char *buff);
    Configuration* ConfigParse(uv_file file);
    Configuration* ConfigParse(std::string path);
};