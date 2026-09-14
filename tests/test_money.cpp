#include <gtest/gtest.h>

#include "lms/money.h"

using namespace lms;

TEST(Money, ParseWholeDollars) {
    EXPECT_EQ(parseMoney("0"), 0);
    EXPECT_EQ(parseMoney("12"), cents(12));
    EXPECT_EQ(parseMoney("100"), cents(100));
}

TEST(Money, ParseWithCents) {
    EXPECT_EQ(parseMoney("12.5"), cents(12, 50));
    EXPECT_EQ(parseMoney("12.50"), cents(12, 50));
    EXPECT_EQ(parseMoney("0.05"), 5);
    EXPECT_EQ(parseMoney(".99"), 99);
}

TEST(Money, ParseRejectsJunk) {
    EXPECT_FALSE(parseMoney("").has_value());
    EXPECT_FALSE(parseMoney("abc").has_value());
    EXPECT_FALSE(parseMoney("12abc").has_value());
    EXPECT_FALSE(parseMoney("-5").has_value());
    EXPECT_FALSE(parseMoney("12.").has_value());
    EXPECT_FALSE(parseMoney("12.345").has_value());
    EXPECT_FALSE(parseMoney("1e5").has_value());
    EXPECT_FALSE(parseMoney(" 12").has_value());
}

TEST(Money, Format) {
    EXPECT_EQ(formatMoney(0), "$0.00");
    EXPECT_EQ(formatMoney(5), "$0.05");
    EXPECT_EQ(formatMoney(cents(12, 50)), "$12.50");
    EXPECT_EQ(formatMoney(cents(1234, 5)), "$1234.05");
    EXPECT_EQ(formatMoney(-cents(3, 7)), "-$3.07");
}

TEST(Money, NoFloatingPointDrift) {
    // 0.1 + 0.2 != 0.3 in double; in cents it is exact.
    Money total = 0;
    for (int i = 0; i < 10; ++i) total += parseMoney("0.10").value();
    EXPECT_EQ(total, cents(1));
    EXPECT_EQ(formatMoney(total), "$1.00");
}
