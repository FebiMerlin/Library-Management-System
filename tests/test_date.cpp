#include <gtest/gtest.h>

#include "lms/date.h"

using namespace lms;

TEST(Date, FormatKnownDays) {
    EXPECT_EQ(formatDay(0), "1970-01-01");
    EXPECT_EQ(formatDay(1), "1970-01-02");
    EXPECT_EQ(formatDay(365), "1971-01-01");
    EXPECT_EQ(formatDay(10957), "2000-01-01");
    EXPECT_EQ(formatDay(20710), "2026-09-14");
    EXPECT_EQ(formatDay(-1), "1969-12-31");
}

TEST(Date, ParseRoundTrip) {
    for (Day d : {0, 1, 59, 60, 365, 10957, 11016, 20710, 25000}) {
        Day parsed = -1;
        ASSERT_TRUE(parseDay(formatDay(d), parsed)) << formatDay(d);
        EXPECT_EQ(parsed, d);
    }
}

TEST(Date, ParseRejectsInvalid) {
    Day d = 0;
    EXPECT_FALSE(parseDay("", d));
    EXPECT_FALSE(parseDay("2026-9-14", d));
    EXPECT_FALSE(parseDay("14.09.2026", d));
    EXPECT_FALSE(parseDay("2026-13-01", d));
    EXPECT_FALSE(parseDay("2026-02-30", d));
    EXPECT_FALSE(parseDay("2026-00-10", d));
    EXPECT_FALSE(parseDay("abcd-ef-gh", d));
}

TEST(Date, LeapYears) {
    Day d = 0;
    EXPECT_TRUE(parseDay("2024-02-29", d));
    EXPECT_FALSE(parseDay("2023-02-29", d));
    EXPECT_TRUE(parseDay("2000-02-29", d));
    EXPECT_FALSE(parseDay("1900-02-29", d));
}

TEST(Date, CurrentDayIsReasonable) {
    const Day today = currentDay();
    EXPECT_GT(today, 20000);  // after 2024-10-04
    EXPECT_LT(today, 40000);  // before 2079
}
