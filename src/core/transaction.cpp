#include "lms/transaction.h"

namespace lms {

const char* toString(TxType type) {
    switch (type) {
        case TxType::AccountOpened: return "OPEN";
        case TxType::Deposit: return "DEPOSIT";
        case TxType::BookIssued: return "ISSUE";
        case TxType::BookReturned: return "RETURN";
        case TxType::Fine: return "FINE";
        case TxType::AccountClosed: return "CLOSE";
    }
    return "?";
}

bool parseTxType(const std::string& text, TxType& out) {
    for (TxType t : {TxType::AccountOpened, TxType::Deposit, TxType::BookIssued, TxType::BookReturned, TxType::Fine,
                     TxType::AccountClosed}) {
        if (text == toString(t)) {
            out = t;
            return true;
        }
    }
    return false;
}

}  // namespace lms
