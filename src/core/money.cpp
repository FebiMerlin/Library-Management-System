#include "lms/money.h"

#include <cctype>

namespace lms {

std::optional<Money> parseMoney(const std::string& text) {
    if (text.empty() || text.size() > 18) return std::nullopt;

    std::size_t i = 0;
    std::int64_t dollars = 0;
    bool anyDigit = false;
    for (; i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])); ++i) {
        dollars = dollars * 10 + (text[i] - '0');
        anyDigit = true;
    }
    std::int64_t frac = 0;
    if (i < text.size() && text[i] == '.') {
        ++i;
        int digits = 0;
        for (; i < text.size() && std::isdigit(static_cast<unsigned char>(text[i])); ++i, ++digits) {
            if (digits < 2) frac = frac * 10 + (text[i] - '0');
            anyDigit = true;
        }
        if (digits == 0 || digits > 2) return std::nullopt;
        if (digits == 1) frac *= 10;
    }
    if (!anyDigit || i != text.size()) return std::nullopt;
    return dollars * 100 + frac;
}

std::string formatMoney(Money value) {
    const bool negative = value < 0;
    if (negative) value = -value;
    std::string s = std::to_string(value / 100);
    const Money c = value % 100;
    s += '.';
    s += static_cast<char>('0' + c / 10);
    s += static_cast<char>('0' + c % 10);
    return (negative ? "-$" : "$") + s;
}

}  // namespace lms
