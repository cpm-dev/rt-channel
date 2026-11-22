#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include "rtchan/counter.hpp"
#include "rtchan/crc8.hpp"

TEST(Crc8SaeJ1850, PublishedVectors) {
    const std::array<std::uint8_t, 9> abcde = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    EXPECT_EQ(rtchan::crc8_sae_j1850(abcde), 0x4Bu);

    const std::array<std::uint8_t, 4> zeros{};
    EXPECT_EQ(rtchan::crc8_sae_j1850(zeros), 0x59u);

    const std::array<std::uint8_t, 3> example = {0xF2u, 0x01u, 0x83u};
    EXPECT_EQ(rtchan::crc8_sae_j1850(example), 0x37u);

    const std::array<std::uint8_t, 4> ones = {0xFFu, 0xFFu, 0xFFu, 0xFFu};
    EXPECT_EQ(rtchan::crc8_sae_j1850(ones), 0x74u);

    const std::array<std::uint8_t, 9> frame = {
        0x33u, 0x22u, 0x55u, 0xAAu, 0xBBu,
        0xCCu, 0xDDu, 0xEEu, 0xFFu};
    EXPECT_EQ(rtchan::crc8_sae_j1850(frame), 0xCBu);
}

TEST(CanFrameCrc, SetAndVerifyByteSeven) {
    std::array<std::uint8_t, 8> frame = {
        0x33u, 0x22u, 0x55u, 0xAAu, 0xBBu, 0xCCu, 0xDDu, 0u};

    ASSERT_TRUE(rtchan::set_can_frame_crc(frame));
    EXPECT_TRUE(rtchan::verify_can_frame_crc(frame));

    frame[3] ^= 0x01u;
    EXPECT_FALSE(rtchan::verify_can_frame_crc(frame));
}

TEST(RollingCounter, WrapsAt15) {
    EXPECT_EQ(rtchan::next_counter(14), 15);
    EXPECT_EQ(rtchan::next_counter(15), 0);
    EXPECT_TRUE(rtchan::counter_continuous(15, 0));
}

TEST(RollingCounter, RejectsGapsDuplicatesAndInvalidValues) {
    EXPECT_TRUE(rtchan::counter_continuous(3, 4));
    EXPECT_FALSE(rtchan::counter_continuous(3, 5));
    EXPECT_FALSE(rtchan::counter_continuous(3, 3));
    EXPECT_FALSE(rtchan::counter_continuous(0x10, 0));
    EXPECT_FALSE(rtchan::counter_continuous(3, 0x10));
}

static_assert(rtchan::next_counter(15) == 0);
static_assert(rtchan::counter_continuous(15, 0));
