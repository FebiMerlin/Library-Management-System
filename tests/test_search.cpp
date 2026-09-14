#include <gtest/gtest.h>

#include "lms/library.h"

using namespace lms;

namespace {

Library makeLibrary() {
    Library lib;
    lib.addBook(1001, "The C++ Programming Language", "Bjarne Stroustrup");
    lib.addBook(1002, "Effective Modern C++", "Scott Meyers");
    lib.addBook(1003, "Clean Code", "Robert C. Martin");
    lib.addBook(2003, "Refactoring", "Martin Fowler");
    return lib;
}

std::vector<int> isbns(const std::vector<Book>& books) {
    std::vector<int> out;
    for (const Book& b : books) out.push_back(b.isbn);
    return out;
}

}  // namespace

TEST(Search, EmptyQueryReturnsAll) {
    Library lib = makeLibrary();
    EXPECT_EQ(lib.searchBooks("").size(), 4u);
}

TEST(Search, ByTitleCaseInsensitive) {
    Library lib = makeLibrary();
    EXPECT_EQ(isbns(lib.searchBooks("c++")), (std::vector<int>{1001, 1002}));
    EXPECT_EQ(isbns(lib.searchBooks("CLEAN")), (std::vector<int>{1003}));
}

TEST(Search, ByAuthor) {
    Library lib = makeLibrary();
    // "Martin" matches both "Robert C. Martin" and "Martin Fowler"
    EXPECT_EQ(isbns(lib.searchBooks("martin")), (std::vector<int>{1003, 2003}));
}

TEST(Search, ByIsbnSubstring) {
    Library lib = makeLibrary();
    EXPECT_EQ(isbns(lib.searchBooks("2003")), (std::vector<int>{2003}));
    EXPECT_EQ(isbns(lib.searchBooks("100")), (std::vector<int>{1001, 1002, 1003}));
}

TEST(Search, NoMatch) {
    Library lib = makeLibrary();
    EXPECT_TRUE(lib.searchBooks("python").empty());
}

TEST(Search, OnlyAvailableFilter) {
    Library lib = makeLibrary();
    ASSERT_TRUE(lib.createAccount(1, "Ivan", cents(100)));
    ASSERT_TRUE(lib.issueBook(1, 1002));
    EXPECT_EQ(isbns(lib.searchBooks("c++", true)), (std::vector<int>{1001}));
    EXPECT_EQ(lib.searchBooks("", true).size(), 3u);
    EXPECT_EQ(lib.searchBooks("", false).size(), 4u);
}
