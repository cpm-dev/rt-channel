#pragma once

#include <bit>
#include <cmath>
#include <cstddef>
#include <expected>
#include <span>

namespace rtchan {

// CAN signal packing and scaling for an 8-byte frame.
// Supports Intel little-endian and Motorola big-endian DBC bit numbering.
// Raw access and encode/decode operations report invalid specifications or values via std::expected.

enum class SignalError {
    InvalidLength,
    OutOfFrame,
    ValueOutOfRange,
};

struct SignalSpec {
    std::size_t start_bit = 0;
    std::size_t length = 0;
    std::endian byte_order = std::endian::little;
    bool is_signed = false;
    double scale = 1.0;
    double offset = 0.0;
    double min = 0.0;
    double max = 0.0;
};

namespace detail {

constexpr bool valid_spec(const SignalSpec& spec) noexcept {
    return spec.length >= 1 && spec.length <= 64;
}

constexpr bool frame_bit(std::size_t start_bit,
                         std::size_t bit_index,
                         std::size_t length,
                         std::endian byte_order,
                         std::size_t& frame_bit) noexcept {
    if (!valid_spec(SignalSpec{start_bit, length, byte_order})) {
        return false;
    }

    if (byte_order == std::endian::little) {
        frame_bit = start_bit + bit_index;
        return frame_bit < 64;
    }

    std::size_t byte = start_bit / 8;
    std::size_t bit = start_bit % 8;
    for (std::size_t i = 0; i < bit_index; ++i) {
        if (bit == 0) {
            ++byte;
            bit = 7;
        } else {
            --bit;
        }
    }
    frame_bit = byte * 8 + bit;
    return frame_bit < 64;
}

constexpr std::uint64_t mask_for_length(std::size_t length) noexcept {
    return length == 64 ? std::uint64_t{0xFFFF'FFFF'FFFF'FFFF}
                        : ((std::uint64_t{1} << length) - 1);
}

constexpr std::uint64_t sign_extend(std::uint64_t raw, std::size_t length) noexcept {
    if (!length || length >= 64) {
        return raw;
    }
    const std::uint64_t sign = std::uint64_t{1} << (length - 1);
    if ((raw & sign) != 0) {
        return raw | ~detail::mask_for_length(length);
    }
    return raw;
}

}  // namespace detail

constexpr std::expected<std::uint64_t, SignalError>
unpack_raw(std::span<const std::uint8_t> frame,
           const SignalSpec& spec) noexcept {
    if (!detail::valid_spec(spec)) {
        return std::unexpected(SignalError::InvalidLength);
    }
    if (frame.size() != 8) {
        return std::unexpected(SignalError::OutOfFrame);
    }

    std::uint64_t raw = 0;
    for (std::size_t i = 0; i < spec.length; ++i) {
        std::size_t position = 0;
        if (!detail::frame_bit(spec.start_bit, i, spec.length,
                               spec.byte_order, position)) {
            return std::unexpected(SignalError::OutOfFrame);
        }
        const std::size_t byte = position / 8;
        const std::size_t bit = position % 8;
        if ((frame[byte] & (std::uint8_t{1} << bit)) != 0) {
            raw |= std::uint64_t{1} << i;
        }
    }

    if (spec.is_signed) {
        raw = detail::sign_extend(raw, spec.length);
    }
    return raw;
}

constexpr std::expected<void, SignalError>
pack_raw(std::span<std::uint8_t> frame,
         const SignalSpec& spec,
         std::uint64_t raw) noexcept {
    if (!detail::valid_spec(spec)) {
        return std::unexpected(SignalError::InvalidLength);
    }
    if (frame.size() != 8) {
        return std::unexpected(SignalError::OutOfFrame);
    }

    for (std::size_t i = 0; i < spec.length; ++i) {
        std::size_t position = 0;
        if (!detail::frame_bit(spec.start_bit, i, spec.length,
                               spec.byte_order, position)) {
            return std::unexpected(SignalError::OutOfFrame);
        }
        const std::size_t byte = position / 8;
        const std::size_t bit = position % 8;
        const std::uint8_t mask =
            static_cast<std::uint8_t>(std::uint8_t{1} << bit);
        if ((raw & (std::uint64_t{1} << i)) != 0) {
            frame[byte] |= mask;
        } else {
            frame[byte] &= static_cast<std::uint8_t>(~mask);
        }
    }
    return {};
}

inline std::expected<std::uint64_t, SignalError>
encode(double physical, const SignalSpec& spec) noexcept {
    if (!detail::valid_spec(spec) || !std::isfinite(physical)) {
        if (!detail::valid_spec(spec)) {
            return std::unexpected(SignalError::InvalidLength);
        }
        return std::unexpected(SignalError::ValueOutOfRange);
    }
    if (physical < spec.min || physical > spec.max) {
        return std::unexpected(SignalError::ValueOutOfRange);
    }
    if (!std::isfinite(spec.scale) || spec.scale == 0.0 ||
        !std::isfinite(spec.offset)) {
        return std::unexpected(SignalError::ValueOutOfRange);
    }

    const double scaled = (physical - spec.offset) / spec.scale;
    if (!std::isfinite(scaled)) {
        return std::unexpected(SignalError::ValueOutOfRange);
    }

    const long double rounded = std::round(static_cast<long double>(scaled));
    if (spec.is_signed) {
        if (spec.length == 64) {
            const long double minimum = -9'223'372'036'854'775'808.0L;
            const long double maximum = 9'223'372'036'854'775'807.0L;
            if (rounded < minimum || rounded > maximum) {
                return std::unexpected(SignalError::ValueOutOfRange);
            }
            const auto signed_value = static_cast<std::int64_t>(rounded);
            return static_cast<std::uint64_t>(signed_value);
        }

        const long double bits = static_cast<long double>(spec.length);
        const long double minimum = -std::pow(2.0L, bits - 1.0L);
        const long double maximum = std::pow(2.0L, bits - 1.0L) - 1.0L;
        if (rounded < minimum || rounded > maximum) {
            return std::unexpected(SignalError::ValueOutOfRange);
        }
        if (rounded < 0.0L) {
            const auto negative = static_cast<std::int64_t>(rounded);
            return static_cast<std::uint64_t>(negative) & detail::mask_for_length(spec.length);
        }
        return static_cast<std::uint64_t>(rounded);
    }

    if (rounded < 0.0L) {
        return std::unexpected(SignalError::ValueOutOfRange);
    }
    const std::uint64_t mask = detail::mask_for_length(spec.length);
    if (rounded > static_cast<long double>(mask)) {
        return std::unexpected(SignalError::ValueOutOfRange);
    }
    return static_cast<std::uint64_t>(rounded);
}

inline std::expected<double, SignalError>
decode(std::span<const std::uint8_t> frame, const SignalSpec& spec) noexcept {
    const auto raw = unpack_raw(frame, spec);
    if (!raw) {
        return std::unexpected(raw.error());
    }

    double value = 0.0;
    if (spec.is_signed) {
        if (spec.length == 64) {
            value = static_cast<double>(static_cast<std::int64_t>(*raw));
        } else {
            const std::uint64_t sign_extended = detail::sign_extend(*raw, spec.length);
            value = static_cast<double>(static_cast<std::int64_t>(sign_extended));
        }
    } else {
        value = static_cast<double>(*raw);
    }

    const double physical = value * spec.scale + spec.offset;
    if (!std::isfinite(physical) || physical < spec.min || physical > spec.max) {
        return std::unexpected(SignalError::ValueOutOfRange);
    }
    return physical;
}

}  // namespace rtchan

