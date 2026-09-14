#pragma once

#include "lms/money.h"

namespace lms {

// Library rules. Values come from the original README ("$20 account opening,
// $30 security deposit, $2 per book for 10 days, fines for late returns").
struct Config {
    Money openingFee = cents(20);
    Money securityDeposit = cents(30);
    Money minInitialDeposit = cents(50);
    Money issueFee = cents(2);
    Money finePerDay = cents(1);
    int loanPeriodDays = 10;
    int maxBooksPerStudent = 3;
    int maxStudents = 20;
    int maxBooks = 100;
};

}  // namespace lms
