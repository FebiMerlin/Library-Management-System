#include <gtest/gtest.h>

#include "lms/library.h"

using namespace lms;

TEST(Books, AddAndFind) {
    Library lib;
    ASSERT_TRUE(lib.addBook(1001, "Clean Code", "Robert C. Martin"));
    const Book* b = lib.findBook(1001);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->title, "Clean Code");
    EXPECT_EQ(b->author, "Robert C. Martin");
    EXPECT_TRUE(b->available());
    EXPECT_EQ(lib.books().size(), 1u);
}

TEST(Books, DuplicateIsbnRejected) {
    Library lib;
    ASSERT_TRUE(lib.addBook(1001, "A", "B"));
    const Result r = lib.addBook(1001, "C", "D");
    EXPECT_FALSE(r);
    EXPECT_NE(r.error.find("already exists"), std::string::npos);
    EXPECT_EQ(lib.books().size(), 1u);
}

TEST(Books, InvalidInputRejected) {
    Library lib;
    EXPECT_FALSE(lib.addBook(0, "A", "B"));
    EXPECT_FALSE(lib.addBook(-5, "A", "B"));
    EXPECT_FALSE(lib.addBook(1, "", "B"));
    EXPECT_FALSE(lib.addBook(1, "A", ""));
    EXPECT_FALSE(lib.addBook(1, "tab\there", "B"));
    EXPECT_FALSE(lib.addBook(1, "A", std::string(101, 'x')));
    EXPECT_TRUE(lib.books().empty());
}

// The original program initialised book_count = MAX_BOOKS, so add_book()
// always failed with "Book limit reached". The limit must only apply once
// the configured number of books really exists.
TEST(Books, LimitAppliesOnlyWhenFull) {
    Config cfg;
    cfg.maxBooks = 2;
    Library lib(cfg);
    EXPECT_TRUE(lib.addBook(1, "A", "B"));
    EXPECT_TRUE(lib.addBook(2, "A", "B"));
    const Result r = lib.addBook(3, "A", "B");
    EXPECT_FALSE(r);
    EXPECT_NE(r.error.find("limit"), std::string::npos);
}

TEST(Books, Edit) {
    Library lib;
    ASSERT_TRUE(lib.addBook(1001, "Old", "Old Author"));
    EXPECT_TRUE(lib.editBook(1001, "New", "New Author"));
    EXPECT_EQ(lib.findBook(1001)->title, "New");
    EXPECT_EQ(lib.findBook(1001)->author, "New Author");
    EXPECT_FALSE(lib.editBook(9999, "X", "Y"));
    EXPECT_FALSE(lib.editBook(1001, "", "Y"));
    EXPECT_EQ(lib.findBook(1001)->title, "New");
}

TEST(Books, Remove) {
    Library lib;
    ASSERT_TRUE(lib.addBook(1001, "A", "B"));
    ASSERT_TRUE(lib.addBook(1002, "C", "D"));
    EXPECT_TRUE(lib.removeBook(1001));
    EXPECT_EQ(lib.findBook(1001), nullptr);
    EXPECT_NE(lib.findBook(1002), nullptr);
    EXPECT_FALSE(lib.removeBook(1001));
}

TEST(Books, RemoveIssuedBookRejected) {
    Library lib;
    ASSERT_TRUE(lib.addBook(1001, "A", "B"));
    ASSERT_TRUE(lib.createAccount(1, "Ivan", cents(100)));
    ASSERT_TRUE(lib.issueBook(1, 1001));
    const Result r = lib.removeBook(1001);
    EXPECT_FALSE(r);
    EXPECT_NE(lib.findBook(1001), nullptr);
}
