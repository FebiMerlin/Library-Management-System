#pragma once

#include <string>

namespace lms {

// Outcome of an operation on the library. Replaces the original approach of
// printing error messages directly from business logic.
struct Result {
    bool ok = true;
    std::string error;

    static Result success() { return {}; }
    static Result failure(std::string message) { return {false, std::move(message)}; }

    explicit operator bool() const { return ok; }
};

}  // namespace lms
