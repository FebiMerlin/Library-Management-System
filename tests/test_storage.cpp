#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

#include "lms/library.h"
#include "lms/storage.h"

using namespace lms;

namespace {

Library makeLibrary() {
    Library lib;
    lib.setToday(20000);
    lib.addBook(1001, "The C++ Programming Language", "Bjarne Stroustrup");
    lib.addBook(1002, "Clean Code", "Robert C. Martin");
    lib.createAccount(230001, "Ivan Petrov", cents(100));
    lib.createAccount(230002, "Anna", cents(75, 50));
    lib.issueBook(230001, 1002);
    lib.deposit(230002, cents(5, 25));
    return lib;
}

void expectSameState(const Library& a, const Library& b) {
    ASSERT_EQ(a.books().size(), b.books().size());
    for (std::size_t i = 0; i < a.books().size(); ++i) {
        EXPECT_EQ(a.books()[i].isbn, b.books()[i].isbn);
        EXPECT_EQ(a.books()[i].title, b.books()[i].title);
        EXPECT_EQ(a.books()[i].author, b.books()[i].author);
        EXPECT_EQ(a.books()[i].issuedTo, b.books()[i].issuedTo);
        EXPECT_EQ(a.books()[i].issuedOn, b.books()[i].issuedOn);
    }
    ASSERT_EQ(a.students().size(), b.students().size());
    for (std::size_t i = 0; i < a.students().size(); ++i) {
        EXPECT_EQ(a.students()[i].roll, b.students()[i].roll);
        EXPECT_EQ(a.students()[i].name, b.students()[i].name);
        EXPECT_EQ(a.students()[i].balance, b.students()[i].balance);
    }
    ASSERT_EQ(a.transactions().size(), b.transactions().size());
    for (std::size_t i = 0; i < a.transactions().size(); ++i) {
        EXPECT_EQ(a.transactions()[i].roll, b.transactions()[i].roll);
        EXPECT_EQ(a.transactions()[i].day, b.transactions()[i].day);
        EXPECT_EQ(a.transactions()[i].type, b.transactions()[i].type);
        EXPECT_EQ(a.transactions()[i].amount, b.transactions()[i].amount);
        EXPECT_EQ(a.transactions()[i].balanceAfter, b.transactions()[i].balanceAfter);
        EXPECT_EQ(a.transactions()[i].isbn, b.transactions()[i].isbn);
    }
}

}  // namespace

TEST(Storage, RoundTripThroughString) {
    const Library original = makeLibrary();
    const std::string text = serialize(original);
    Library restored;
    ASSERT_TRUE(deserialize(text, restored));
    expectSameState(original, restored);
    EXPECT_EQ(serialize(restored), text);
}

TEST(Storage, SerializedFormatIsStable) {
    Library lib;
    lib.setToday(5);
    lib.addBook(7, "T", "A");
    lib.createAccount(3, "N", cents(50));
    const std::string expected =
        "LMS\t1\n"
        "BOOK\t7\tT\tA\t0\t0\n"
        "STUDENT\t3\tN\t0\n"
        "TX\t3\t5\tOPEN\t0\t0\t0\n";
    EXPECT_EQ(serialize(lib), expected);
}

TEST(Storage, RoundTripThroughFile) {
    const std::string path = "lms_test_roundtrip.dat";
    const Library original = makeLibrary();
    ASSERT_TRUE(saveToFile(original, path));
    Library restored;
    ASSERT_TRUE(loadFromFile(restored, path));
    expectSameState(original, restored);
    std::remove(path.c_str());
}

TEST(Storage, MissingFileIsNotAnError) {
    Library lib;
    EXPECT_TRUE(loadFromFile(lib, "definitely_missing_file.dat"));
    EXPECT_TRUE(lib.books().empty());
}

TEST(Storage, RejectsUnknownFormat) {
    Library lib;
    EXPECT_FALSE(deserialize("", lib));
    EXPECT_FALSE(deserialize("HELLO\n", lib));
    EXPECT_FALSE(deserialize("LMS\t2\n", lib));
}

TEST(Storage, RejectsCorruptRecords) {
    Library lib;
    EXPECT_FALSE(deserialize("LMS\t1\nBOOK\tabc\tT\tA\t0\t0\n", lib));
    EXPECT_FALSE(deserialize("LMS\t1\nBOOK\t1\tT\n", lib));
    EXPECT_FALSE(deserialize("LMS\t1\nSTUDENT\t1\tN\tmoney\n", lib));
    EXPECT_FALSE(deserialize("LMS\t1\nTX\t1\t0\tBOGUS\t0\t0\t0\n", lib));
    EXPECT_FALSE(deserialize("LMS\t1\nWHATEVER\t1\n", lib));
    const Result r = deserialize("LMS\t1\nBOOK\t1\tT\tA\t0\t0\nBOOK\t1\tT\tA\t0\t0\n", lib);
    EXPECT_FALSE(r);
    EXPECT_NE(r.error.find("line 3"), std::string::npos);
    EXPECT_TRUE(lib.books().empty());  // nothing applied on failure
}

TEST(Storage, RejectsBookIssuedToUnknownStudent) {
    Library lib;
    EXPECT_FALSE(deserialize("LMS\t1\nBOOK\t1\tT\tA\t42\t100\n", lib));
}

TEST(Storage, ToleratesWindowsLineEndings) {
    Library lib;
    ASSERT_TRUE(deserialize("LMS\t1\r\nBOOK\t7\tT\tA\t0\t0\r\n\r\n", lib));
    ASSERT_EQ(lib.books().size(), 1u);
    EXPECT_EQ(lib.books()[0].author, "A");
}

TEST(Storage, DefaultCatalogue) {
    Library lib;
    seedDefaultCatalogue(lib);
    EXPECT_EQ(lib.books().size(), 15u);
    for (const Book& b : lib.books()) {
        EXPECT_NE(b.title, "Title");  // no placeholders
        EXPECT_TRUE(b.available());
    }
}
