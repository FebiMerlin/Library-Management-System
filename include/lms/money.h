#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace lms {

// Money is stored as an integer number of cents to avoid floating-point
// rounding errors (the original code used `double` for balances).
using Money = std::int64_t;

constexpr Money cents(std::int64_t dollars, std::int64_t c = 0) {
    return dollars * 100 + c;
}

// "12.50" -> 1250. Accepts "12", "12.5", "12.50". Rejects negatives and junk.
std::optional<Money> parseMoney(const std::string& text);

// 1250 -> "$12.50"
std::string formatMoney(Money value);

}  // namespace lms
