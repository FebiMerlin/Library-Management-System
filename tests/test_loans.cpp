#include <gtest/gtest.h>

#include "lms/library.h"

using namespace lms;

namespace {

Library makeLibrary() {
    Library lib;
    lib.setToday(1000);
    lib.addBook(1001, "A", "X");
    lib.addBook(1002, "B", "Y");
    lib.addBook(1003, "C", "Z");
    lib.addBook(1004, "D", "W");
    lib.createAccount(1, "Ivan", cents(100));  // balance $50
    lib.createAccount(2, "Anna", cents(50));   // balance $0
    return lib;
}

}  // namespace

TEST(Loans, IssueChargesFeeAndMarksBook) {
    Library lib = makeLibrary();
    ASSERT_TRUE(lib.issueBook(1, 1001));
    EXPECT_EQ(lib.findStudent(1)->balance, cents(48));
    const Book* b = lib.findBook(1001);
    EXPECT_FALSE(b->available());
    EXPECT_EQ(b->issuedTo, 1);
    EXPECT_EQ(b->issuedOn, 1000);
    ASSERT_EQ(lib.booksIssuedTo(1).size(), 1u);
    EXPECT_EQ(lib.booksIssuedTo(1)[0].isbn, 1001);
}

TEST(Loans, IssueRejectsUnknownStudentOrBook) {
    Library lib = makeLibrary();
    EXPECT_FALSE(lib.issueBook(99, 1001));
    // The original indexed book_available[choice - 1] without bounds checks.
    EXPECT_FALSE(lib.issueBook(1, 9999));
    EXPECT_FALSE(lib.issueBook(1, 0));
    EXPECT_FALSE(lib.issueBook(1, -1));
    EXPECT_EQ(lib.findStudent(1)->balance, cents(50));
}

TEST(Loans, IssueRejectsUnavailableBook) {
    Library lib = makeLibrary();
    ASSERT_TRUE(lib.issueBook(1, 1001));
    ASSERT_TRUE(lib.deposit(2, cents(10)));
    const Result r = lib.issueBook(2, 1001);
    EXPECT_FALSE(r);
    EXPECT_NE(r.error.find("unavailable"), std::string::npos);
}

TEST(Loans, IssueRejectsInsufficientBalance) {
    Library lib = makeLibrary();
    const Result r = lib.issueBook(2, 1001);
    EXPECT_FALSE(r);
    EXPECT_NE(r.error.find("Insufficient"), std::string::npos);
    EXPECT_TRUE(lib.findBook(1001)->available());
}

TEST(Loans, MaxBooksPerStudent) {
    Library lib = makeLibrary();
    EXPECT_TRUE(lib.issueBook(1, 1001));
    EXPECT_TRUE(lib.issueBook(1, 1002));
    EXPECT_TRUE(lib.issueBook(1, 1003));
    const Result r = lib.issueBook(1, 1004);
    EXPECT_FALSE(r);
    EXPECT_NE(r.error.find("maximum"), std::string::npos);
    EXPECT_EQ(lib.booksIssuedTo(1).size(), 3u);
}

TEST(Loans, ReturnOnTimeHasNoFine) {
    Library lib = makeLibrary();
    ASSERT_TRUE(lib.issueBook(1, 1001));
    lib.setToday(1010);  // exactly the loan period
    Money fine = -1;
    ASSERT_TRUE(lib.returnBook(1, 1001, &fine));
    EXPECT_EQ(fine, 0);
    EXPECT_TRUE(lib.findBook(1001)->available());
    EXPECT_EQ(lib.findStudent(1)->balance, cents(48));
    EXPECT_TRUE(lib.booksIssuedTo(1).empty());
}

TEST(Loans, LateReturnIsFinedPerDay) {
    Library lib = makeLibrary();
    ASSERT_TRUE(lib.issueBook(1, 1001));
    lib.setToday(1013);  // 3 days late
    EXPECT_EQ(lib.daysOverdue(*lib.findBook(1001)), 3);
    Money fine = 0;
    ASSERT_TRUE(lib.returnBook(1, 1001, &fine));
    EXPECT_EQ(fine, cents(3));
    EXPECT_EQ(lib.findStudent(1)->balance, cents(45));
}

TEST(Loans, FineCanMakeBalanceNegative) {
    Library lib = makeLibrary();
    ASSERT_TRUE(lib.issueBook(1, 1001));
    lib.setToday(1000 + 10 + 60);
    Money fine = 0;
    ASSERT_TRUE(lib.returnBook(1, 1001, &fine));
    EXPECT_EQ(fine, cents(60));
    EXPECT_EQ(lib.findStudent(1)->balance, cents(48) - cents(60));
}

TEST(Loans, ReturnRejectsWrongStudent) {
    Library lib = makeLibrary();
    ASSERT_TRUE(lib.issueBook(1, 1001));
    EXPECT_FALSE(lib.returnBook(2, 1001));
    EXPECT_FALSE(lib.returnBook(1, 1002));  // not issued at all
    EXPECT_FALSE(lib.returnBook(1, 9999));
    EXPECT_FALSE(lib.findBook(1001)->available());
}

TEST(Loans, OverdueReport) {
    Library lib = makeLibrary();
    ASSERT_TRUE(lib.issueBook(1, 1001));
    lib.setToday(1005);
    ASSERT_TRUE(lib.issueBook(1, 1002));
    lib.setToday(1012);
    const auto overdue = lib.overdueBooks();
    ASSERT_EQ(overdue.size(), 1u);
    EXPECT_EQ(overdue[0].isbn, 1001);
    EXPECT_EQ(lib.daysOverdue(overdue[0]), 2);
    EXPECT_EQ(lib.daysOverdue(*lib.findBook(1002)), 0);
    EXPECT_EQ(lib.daysOverdue(*lib.findBook(1003)), 0);
}
