#pragma once

#include <string>

namespace lms {

using Day = int;  // days since Unix epoch (UTC); 0 means "not set"

struct Book {
    int isbn = 0;
    std::string title;
    std::string author;
    int issuedTo = 0;  // roll number of the borrower, 0 if the book is on the shelf
    Day issuedOn = 0;  // day the book was issued, 0 if available

    bool available() const { return issuedTo == 0; }
};

}  // namespace lms
