#include <gtest/gtest.h>
#include <ArduinoJson.h>
#include "imhr_departure_logic.h"
#include "imhr_display_logic.h"

using namespace imhr;

TEST(GetDelayInMinutes, ReturnsZeroWhenDelayIsMissing)
{
    JsonDocument doc;
    JsonObject departure = doc.to<JsonObject>();

    EXPECT_EQ(getDelayInMinutes(departure), 0);
}

TEST(GetDelayInMinutes, ConvertsSecondsToMinutes)
{
    JsonDocument doc;
    doc["delay"] = 180;
    JsonObject departure = doc.as<JsonObject>();

    EXPECT_EQ(getDelayInMinutes(departure), 3);
}

TEST(GetDelayInMinutes, RoundsDownPartialMinutes)
{
    JsonDocument doc;
    doc["delay"] = 119;
    JsonObject departure = doc.as<JsonObject>();

    EXPECT_EQ(getDelayInMinutes(departure), 1);
}

TEST(FormatPlatformLabel, PrefersRealtimePlatformAndStripsPrefix)
{
    JsonDocument doc;
    doc["line"]["name"] = "S3";
    doc["realtimePlatform"] = "Gleis 12";
    doc["platform"] = "Gleis 3";
    JsonObject departure = doc.as<JsonObject>();

    EXPECT_EQ(formatPlatformLabel(departure), "S3-12");
}

TEST(FormatPlatformLabel, FallsBackToPlatformWhenRealtimeMissing)
{
    JsonDocument doc;
    doc["line"]["name"] = "U1";
    doc["platform"] = "Gleis 4";
    JsonObject departure = doc.as<JsonObject>();

    EXPECT_EQ(formatPlatformLabel(departure), "U1-4");
}

TEST(FormatPlatformLabel, ReturnsJustLineNameWhenNoPlatform)
{
    JsonDocument doc;
    doc["line"]["name"] = "19";
    JsonObject departure = doc.as<JsonObject>();

    EXPECT_EQ(formatPlatformLabel(departure), "19");
}

TEST(DetermineMinuteDisplay, ShowsDashesForNegativeMinutes)
{
    char buf[12];
    determineMinuteDisplay(buf, sizeof(buf), -1);
    EXPECT_STREQ(buf, "--");
}

TEST(DetermineMinuteDisplay, ShowsSofortForZeroMinutes)
{
    char buf[12];
    determineMinuteDisplay(buf, sizeof(buf), 0);
    EXPECT_STREQ(buf, "sofort");
}

TEST(DetermineMinuteDisplay, ShowsMinutesCount)
{
    char buf[12];
    determineMinuteDisplay(buf, sizeof(buf), 7);
    EXPECT_STREQ(buf, "7 min");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
