#pragma once

#include <string>

#include "lms/money.h"

namespace lms {

struct Student {
    int roll = 0;
    std::string name;
    Money balance = 0;
};

}  // namespace lms
