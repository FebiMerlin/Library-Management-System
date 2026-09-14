#pragma once

#include <string>

#include "lms/book.h"

namespace lms {

// Days since 1970-01-01 for the current UTC date.
Day currentDay();

// "2026-09-14" for the given day number.
std::string formatDay(Day day);

// Inverse of formatDay: returns false if the text is not a valid YYYY-MM-DD.
bool parseDay(const std::string& text, Day& out);

}  // namespace lms
