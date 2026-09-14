#include <gtest/gtest.h>

#include "lms/library.h"

using namespace lms;

TEST(Students, CreateAccountChargesFees) {
    Library lib;
    ASSERT_TRUE(lib.createAccount(230001, "Ivan Petrov", cents(100)));
    const Student* s = lib.findStudent(230001);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->name, "Ivan Petrov");
    // $100 - $20 opening fee - $30 security deposit
    EXPECT_EQ(s->balance, cents(50));
}

TEST(Students, MinimumDepositEnforced) {
    Library lib;
    EXPECT_FALSE(lib.createAccount(1, "A", cents(49, 99)));
    EXPECT_TRUE(lib.createAccount(1, "A", cents(50)));
    EXPECT_EQ(lib.findStudent(1)->balance, 0);
}

// In the original, a failed creation left the roll number and name written
// into the arrays. Here a rejected account must leave no trace.
TEST(Students, FailedCreationLeavesNoTrace) {
    Library lib;
    EXPECT_FALSE(lib.createAccount(1, "A", cents(10)));
    EXPECT_EQ(lib.findStudent(1), nullptr);
    EXPECT_TRUE(lib.students().empty());
    EXPECT_TRUE(lib.transactions().empty());
}

TEST(Students, DuplicateRollRejected) {
    Library lib;
    ASSERT_TRUE(lib.createAccount(1, "A", cents(50)));
    EXPECT_FALSE(lib.createAccount(1, "B", cents(50)));
    EXPECT_EQ(lib.students().size(), 1u);
}

TEST(Students, InvalidRollOrNameRejected) {
    Library lib;
    EXPECT_FALSE(lib.createAccount(0, "A", cents(50)));
    EXPECT_FALSE(lib.createAccount(-1, "A", cents(50)));
    EXPECT_FALSE(lib.createAccount(1, "", cents(50)));
    EXPECT_FALSE(lib.createAccount(1, "line\nbreak", cents(50)));
}

TEST(Students, StudentLimit) {
    Config cfg;
    cfg.maxStudents = 2;
    Library lib(cfg);
    EXPECT_TRUE(lib.createAccount(1, "A", cents(50)));
    EXPECT_TRUE(lib.createAccount(2, "B", cents(50)));
    EXPECT_FALSE(lib.createAccount(3, "C", cents(50)));
}

TEST(Students, Deposit) {
    Library lib;
    ASSERT_TRUE(lib.createAccount(1, "A", cents(50)));
    EXPECT_TRUE(lib.deposit(1, cents(12, 34)));
    EXPECT_EQ(lib.findStudent(1)->balance, cents(12, 34));
    EXPECT_FALSE(lib.deposit(1, 0));
    EXPECT_FALSE(lib.deposit(1, -cents(1)));  // original accepted negative deposits
    EXPECT_FALSE(lib.deposit(2, cents(1)));
    EXPECT_EQ(lib.findStudent(1)->balance, cents(12, 34));
}

TEST(Students, SortedListDoesNotReorderStorage) {
    Library lib;
    ASSERT_TRUE(lib.createAccount(30, "C", cents(50)));
    ASSERT_TRUE(lib.createAccount(10, "A", cents(50)));
    ASSERT_TRUE(lib.createAccount(20, "B", cents(50)));
    const auto sorted = lib.studentsSortedByRoll();
    ASSERT_EQ(sorted.size(), 3u);
    EXPECT_EQ(sorted[0].roll, 10);
    EXPECT_EQ(sorted[1].roll, 20);
    EXPECT_EQ(sorted[2].roll, 30);
    EXPECT_EQ(sorted[0].name, "A");
    EXPECT_EQ(lib.students()[0].roll, 30);  // original order untouched
}

TEST(Students, CloseAccountRefundsBalanceAndDeposit) {
    Library lib;
    ASSERT_TRUE(lib.createAccount(1, "A", cents(100)));  // balance 50
    Money refund = 0;
    EXPECT_TRUE(lib.closeAccount(1, &refund));
    EXPECT_EQ(refund, cents(80));  // 50 balance + 30 security deposit
    EXPECT_EQ(lib.findStudent(1), nullptr);
    EXPECT_FALSE(lib.closeAccount(1));
}

TEST(Students, CloseAccountWithBooksRejected) {
    Library lib;
    ASSERT_TRUE(lib.addBook(1001, "A", "B"));
    ASSERT_TRUE(lib.createAccount(1, "A", cents(100)));
    ASSERT_TRUE(lib.issueBook(1, 1001));
    EXPECT_FALSE(lib.closeAccount(1));
    ASSERT_TRUE(lib.returnBook(1, 1001));
    EXPECT_TRUE(lib.closeAccount(1));
}
