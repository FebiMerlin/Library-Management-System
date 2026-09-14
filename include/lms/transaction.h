#pragma once

#include <string>

#include "lms/book.h"
#include "lms/money.h"

namespace lms {

enum class TxType { AccountOpened, Deposit, BookIssued, BookReturned, Fine, AccountClosed };

const char* toString(TxType type);
bool parseTxType(const std::string& text, TxType& out);

// One line of a student's account statement (feature: transaction history).
struct Transaction {
    int roll = 0;
    Day day = 0;
    TxType type = TxType::Deposit;
    Money amount = 0;  // signed: positive credits the account, negative debits it
    Money balanceAfter = 0;
    int isbn = 0;  // 0 when the transaction is not about a book
};

}  // namespace lms
