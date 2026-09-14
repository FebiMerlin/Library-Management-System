#include <gtest/gtest.h>

#include "lms/library.h"

using namespace lms;

TEST(History, RecordsEveryOperationInOrder) {
    Library lib;
    lib.setToday(100);
    ASSERT_TRUE(lib.addBook(1001, "A", "B"));
    ASSERT_TRUE(lib.createAccount(1, "Ivan", cents(100)));  // balance 50
    ASSERT_TRUE(lib.deposit(1, cents(10)));                 // 60
    ASSERT_TRUE(lib.issueBook(1, 1001));                    // 58
    lib.setToday(115);                                      // 5 days late
    ASSERT_TRUE(lib.returnBook(1, 1001));                   // 53

    const auto h = lib.history(1);
    ASSERT_EQ(h.size(), 5u);

    EXPECT_EQ(h[0].type, TxType::AccountOpened);
    EXPECT_EQ(h[0].amount, cents(50));
    EXPECT_EQ(h[0].balanceAfter, cents(50));
    EXPECT_EQ(h[0].day, 100);

    EXPECT_EQ(h[1].type, TxType::Deposit);
    EXPECT_EQ(h[1].amount, cents(10));
    EXPECT_EQ(h[1].balanceAfter, cents(60));

    EXPECT_EQ(h[2].type, TxType::BookIssued);
    EXPECT_EQ(h[2].amount, -cents(2));
    EXPECT_EQ(h[2].balanceAfter, cents(58));
    EXPECT_EQ(h[2].isbn, 1001);

    EXPECT_EQ(h[3].type, TxType::BookReturned);
    EXPECT_EQ(h[3].amount, 0);
    EXPECT_EQ(h[3].isbn, 1001);
    EXPECT_EQ(h[3].day, 115);

    EXPECT_EQ(h[4].type, TxType::Fine);
    EXPECT_EQ(h[4].amount, -cents(5));
    EXPECT_EQ(h[4].balanceAfter, cents(53));
}

TEST(History, IsPerStudent) {
    Library lib;
    ASSERT_TRUE(lib.createAccount(1, "A", cents(50)));
    ASSERT_TRUE(lib.createAccount(2, "B", cents(50)));
    ASSERT_TRUE(lib.deposit(2, cents(1)));
    EXPECT_EQ(lib.history(1).size(), 1u);
    EXPECT_EQ(lib.history(2).size(), 2u);
    EXPECT_TRUE(lib.history(3).empty());
    EXPECT_EQ(lib.transactions().size(), 3u);
}

TEST(History, FailedOperationsAreNotRecorded) {
    Library lib;
    ASSERT_TRUE(lib.createAccount(1, "A", cents(50)));
    EXPECT_FALSE(lib.deposit(1, 0));
    EXPECT_FALSE(lib.issueBook(1, 9999));
    EXPECT_FALSE(lib.returnBook(1, 9999));
    EXPECT_EQ(lib.history(1).size(), 1u);
}

TEST(History, ClosingAccountIsRecorded) {
    Library lib;
    ASSERT_TRUE(lib.createAccount(1, "A", cents(100)));
    ASSERT_TRUE(lib.closeAccount(1));
    const auto h = lib.history(1);
    ASSERT_EQ(h.size(), 2u);
    EXPECT_EQ(h[1].type, TxType::AccountClosed);
    EXPECT_EQ(h[1].amount, -cents(80));
    EXPECT_EQ(h[1].balanceAfter, 0);
}

TEST(History, TxTypeNamesRoundTrip) {
    for (TxType t : {TxType::AccountOpened, TxType::Deposit, TxType::BookIssued, TxType::BookReturned, TxType::Fine,
                     TxType::AccountClosed}) {
        TxType parsed = TxType::Deposit;
        ASSERT_TRUE(parseTxType(toString(t), parsed));
        EXPECT_EQ(parsed, t);
    }
    TxType parsed;
    EXPECT_FALSE(parseTxType("nope", parsed));
}
