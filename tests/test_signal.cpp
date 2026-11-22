#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include "rtchan/signal.hpp"

TEST(Signal, LittleEndianPacking) {
    std::array<std::uint8_t, 8> frame{};
    rtchan::SignalSpec spec{.start_bit = 1, .length = 8,
                            .byte_order = std::endian::little};

    ASSERT_TRUE(rtchan::pack_raw(frame, spec, 0xA5).has_value());
    EXPECT_EQ(frame[0], 0x4A);
    EXPECT_EQ(frame[1], 0x01);

    const auto raw = rtchan::unpack_raw(frame, spec);
    ASSERT_TRUE(raw.has_value());
    EXPECT_EQ(*raw, 0xA5u);
}

TEST(Signal, BigEndianSawtoothPacking) {
    std::array<std::uint8_t, 8> frame{};
    rtchan::SignalSpec spec{.start_bit = 3, .length = 8,
                            .byte_order = std::endian::big};

    ASSERT_TRUE(rtchan::pack_raw(frame, spec, 0xA5).has_value());
    EXPECT_EQ(frame[0], 0x0A);
    EXPECT_EQ(frame[1], 0x50);

    const auto raw = rtchan::unpack_raw(frame, spec);
    ASSERT_TRUE(raw.has_value());
    EXPECT_EQ(*raw, 0xA5u);
}

TEST(Signal, SignedDecodeAndEncode) {
    rtchan::SignalSpec spec{.start_bit = 0, .length = 8,
                            .byte_order = std::endian::little,
                            .is_signed = true, .scale = 0.5,
                            .offset = 10.0, .min = 6.0, .max = 18.0};

    std::array<std::uint8_t, 8> frame{};
    ASSERT_TRUE(rtchan::pack_raw(frame, spec, 0xFCu).has_value());

    const auto decoded = rtchan::decode(frame, spec);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_DOUBLE_EQ(*decoded, 8.0);

    const auto encoded = rtchan::encode(8.0, spec);
    ASSERT_TRUE(encoded.has_value());
    EXPECT_EQ(*encoded & 0xFFu, 0xFCu);
}

TEST(Signal, RejectsInvalidLengthAndOutOfFrame) {
    std::array<std::uint8_t, 8> frame{};

    rtchan::SignalSpec invalid{.start_bit = 0, .length = 0,
                               .byte_order = std::endian::little};
    const auto invalid_result = rtchan::pack_raw(frame, invalid, 0);
    ASSERT_FALSE(invalid_result.has_value());
    EXPECT_EQ(invalid_result.error(), rtchan::SignalError::InvalidLength);

    rtchan::SignalSpec outside{.start_bit = 63, .length = 2,
                               .byte_order = std::endian::little};
    const auto outside_result = rtchan::pack_raw(frame, outside, 0);
    ASSERT_FALSE(outside_result.has_value());
    EXPECT_EQ(outside_result.error(), rtchan::SignalError::OutOfFrame);
}

constexpr std::array<std::uint8_t, 8> makeLittleEndianFrame() {
    std::array<std::uint8_t, 8> frame{};
    rtchan::SignalSpec spec{.start_bit = 0, .length = 8,
                            .byte_order = std::endian::little};
    const auto result = rtchan::pack_raw(frame, spec, 0x5A);
    if (!result) return {};
    return frame;
}

static_assert(makeLittleEndianFrame()[0] == 0x5A);
