#include "console.h"

#include <cctype>

namespace lms::cli {

namespace {

std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

bool parseIntStrict(const std::string& s, long long& out) {
    if (s.empty() || s.size() > 11) return false;
    std::size_t i = (s[0] == '-') ? 1 : 0;
    if (i == s.size()) return false;
    long long v = 0;
    for (; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
        v = v * 10 + (s[i] - '0');
    }
    out = s[0] == '-' ? -v : v;
    return true;
}

}  // namespace

std::optional<std::string> Console::readLine(const std::string& prompt) {
    out_ << prompt << std::flush;
    std::string line;
    if (!std::getline(in_, line)) {
        out_ << '\n';
        return std::nullopt;
    }
    return trim(line);
}

std::optional<std::string> Console::readNonEmpty(const std::string& prompt) {
    for (;;) {
        auto line = readLine(prompt);
        if (!line) return std::nullopt;
        if (!line->empty()) return line;
        out_ << "Value must not be empty.\n";
    }
}

std::optional<int> Console::readInt(const std::string& prompt, int min, int max) {
    for (;;) {
        auto line = readLine(prompt);
        if (!line) return std::nullopt;
        long long v = 0;
        if (parseIntStrict(*line, v) && v >= min && v <= max) return static_cast<int>(v);
        out_ << "Please enter a whole number";
        if (min != 0 || max != 2147483647) out_ << " between " << min << " and " << max;
        out_ << ".\n";
    }
}

std::optional<Money> Console::readMoney(const std::string& prompt) {
    for (;;) {
        auto line = readLine(prompt);
        if (!line) return std::nullopt;
        std::string text = *line;
        if (!text.empty() && text[0] == '$') text.erase(0, 1);
        if (auto m = parseMoney(text)) return m;
        out_ << "Please enter an amount like 12 or 12.50.\n";
    }
}

std::optional<bool> Console::readYesNo(const std::string& prompt) {
    for (;;) {
        auto line = readLine(prompt + " (y/n): ");
        if (!line) return std::nullopt;
        if (*line == "y" || *line == "Y" || *line == "yes") return true;
        if (*line == "n" || *line == "N" || *line == "no") return false;
        out_ << "Please answer y or n.\n";
    }
}

}  // namespace lms::cli
