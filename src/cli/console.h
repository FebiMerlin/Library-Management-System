#pragma once

#include <iostream>
#include <optional>
#include <string>

#include "lms/money.h"

namespace lms::cli {

// Thin wrapper over std::cin/std::cout that validates every input.
// The original program did `cin >> int` everywhere: a non-numeric answer put
// the stream into a failed state and silently turned the value into 0.
class Console {
public:
    Console(std::istream& in, std::ostream& out) : in_(in), out_(out) {}

    std::ostream& out() { return out_; }

    // Each reader returns std::nullopt only on end-of-input (Ctrl+D / Ctrl+Z
    // or the end of a piped script); invalid answers are re-asked.
    std::optional<std::string> readLine(const std::string& prompt);
    std::optional<std::string> readNonEmpty(const std::string& prompt);
    std::optional<int> readInt(const std::string& prompt, int min = 0, int max = 2147483647);
    std::optional<Money> readMoney(const std::string& prompt);
    std::optional<bool> readYesNo(const std::string& prompt);

private:
    std::istream& in_;
    std::ostream& out_;
};

}  // namespace lms::cli
